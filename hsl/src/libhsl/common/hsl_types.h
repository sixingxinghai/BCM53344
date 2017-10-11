/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HSL_TYPES_H_
#define _HSL_TYPES_H_

#define ICMPV6_PROTOCOL           0x3a
#define VRRP_PROTOCOL             0x70

/* Number of bits in prefix type. */
#ifndef PNBBY
#define PNBBY 8
#endif /* PNBBY */

/* Maximum of two values */
#ifndef MAX
#define MAX(a,b)    ((a) > (b) ? (a) : (b))
#endif /* !MAX */

/* Minimum of two comparable values */
#ifndef MIN
#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#endif /* !MIN */

#if defined VXWORKS && !(defined PNE_VERSION_2_2)
typedef unsigned int       u_int32_t; /* 32 bit unsigned integer */
typedef unsigned short     u_int16_t; /* 16 bit unsigned integer */
typedef unsigned char      u_int8_t;  /* 8 bit unsigned integer */
#endif /* VXWORKS && ! PNE_VERSION_2_2 */

#if defined HAVE_USER_HSL && !defined __KERNEL__
#include "pal.h"
#else
typedef signed int         s_int32_t; /* 32 bit signed integer */
typedef signed short       s_int16_t; /* 16 bit signed integer */
typedef signed char        s_int8_t;   /* 8 bit signed integer */

typedef union {
        u_int8_t  c[8];
        u_int16_t s[4];
        u_int32_t l[2];
} ut_int64_t;                    /* 64 bit unsigned integer */

typedef union {
        s_int8_t  c[8];
        s_int16_t s[4];
        s_int32_t l[2];
} st_int64_t;                    /* 64 bit signed integer */

#endif /* HAVE_USER_HSL */

#define ADD_64_UINT(A,B,RESULT)           \
   do {                                   \
   RESULT.l[0] = A.l[0] + B.l[0];         \
   RESULT.l[1] = A.l[1] + B.l[1];         \
   RESULT.l[1] += (RESULT.l[0] < MAX(A.l[0],B.l[0]))?1:0; \
  } while (0)

#define SUB_64_UINT(A,B,RESULT)              \
   do {                                      \
   ut_int64_t *_res = &RESULT;               \
   if((A.l[1] < B.l[1]) ||                   \
      ((A.l[1] == B.l[1]) &&                 \
       (A.l[0] < B.l[0])))                   \
     {                                       \
        _res->l[0] = 0xFFFFFFFF - A.l[0];    \
        _res->l[1] = 0xFFFFFFFF - A.l[1];    \
        ADD_64_UINT(RESULT,B,RESULT);        \
     }                                       \
   else                                      \
     {                                       \
        if(A.l[0] < B.l[0])                  \
          {                                  \
             _res->l[0] = (0xFFFFFFFF -      \
                     (B.l[0] - A.l[0] - 1)); \
              A.l[1] -= 1;                   \
          }                                  \
        else                                 \
          {                                  \
             _res->l[0] = (A.l[0] - B.l[0]); \
          }                                  \
        _res->l[1] = A.l[1] - B.l[1];        \
     }                                       \
  } while (0)

/* Flag manipulation macros. */
#define CHECK_FLAG(V,F)      ((V) & (F))
#define SET_FLAG(V,F)        (V) = (V) | (F)
#define UNSET_FLAG(V,F)      (V) = (V) & ~(F)
#define FLAG_ISSET(V,F)      (((V) & (F)) == (F))


/* db/fib operation types. */
#define HSL_OPER_ADD              (1 << 0)
#define HSL_OPER_DELETE           (1 << 1)
#define HSL_OPER_GET              (1 << 2)

#define IP_HEADER_SIZE              20
#define IPV6_HEADER_SIZE            40

#define VRRP_OFFSET_FOR_IP_PKT      ENET_UNTAGGED_HDR_LEN + IP_HEADER_SIZE
#define VRRP_OFFSET_FOR_IPV6_PKT    ENET_UNTAGGED_HDR_LEN + IPV6_HEADER_SIZE

/* 
   Ethernet LAN interface address. 
*/
#define HSL_ETHER_ALEN          6
typedef u_char hsl_mac_address_t[HSL_ETHER_ALEN];

/*
  Interface identifier. 
*/
typedef u_int32_t hsl_ifIndex_t;

/*
  VID. 
*/
typedef u_int16_t hsl_vid_t;
#define HSL_DEFAULT_VID                1

/* 
   IPv4 address. 
*/
typedef u_int32_t hsl_ipv4Address_t;

/* 
   IPv4 prefix. 
*/
typedef struct 
{
  u_char family;
  u_char prefixlen;
  u_char pad1;
  u_char pad2;
  hsl_ipv4Address_t address;
} hsl_ipv4Prefix_t;

#define HSL_INADDR_LOOPBACK   (unsigned long)0x7f000001U
#define HSL_LOOPBACK_PREFIXLEN   8

#ifdef HAVE_IPV6
/* 
   IPv6 address. 
*/
typedef struct 
{
  hsl_ipv4Address_t word[4];
} hsl_ipv6Address_t;

#define HSL_IPV6_LOOPBACK_ADDR "::1"
#define HSL_IPV6_LOOPBACK_PREFIXLEN  128
#define HSL_IPV6_LINKLOCAL_LOOPBACK_ADDR "fe80::1"
#define HSL_IPV6_LINKLOCAL_LOOPBACK_PREFIXLEN  64 


/* IPv6 prefix length. */
typedef u_char hsl_ipv6Prefixlen_t;

/* Ipv6 Link local prefix length */
#define HSL_IPV6_LINKLOCAL_PREFIXLEN  64

/* 
   IPv6 prefix. 
*/
typedef struct 
{
  u_char family;
  u_char prefixlen;
  u_char pad1;
  u_char pad2;
  hsl_ipv6Address_t address;
} hsl_ipv6Prefix_t;

#endif /* HAVE_IPV6 */

/*
  Unified prefix structure. 
*/
typedef struct 
{
  u_char family;
  u_char prefixlen;
  u_char pad1;
  u_char pad2;
  union 
  {
    u_char prefix;
    hsl_ipv4Address_t prefix4;
#ifdef HAVE_IPV6
    hsl_ipv6Address_t prefix6;
#endif /* HAVE_IPV6 */
  } u;
} hsl_prefix_t;

/*
  IP header. 
*/
/*
  IPv4 header.
*/
struct hsl_ip
{
#if defined(WORDS_LITTLEENDIAN)
  u_char ip_hl:4;                             /* Header length. */
  u_char ip_v:4;                              /* Version. */
#elif defined(WORDS_BIGENDIAN)
  u_char ip_v:4;                              /* Version. */
  u_char ip_hl:4;                             /* Header length. */
#endif
  u_char ip_tos;                              /* ToS. */
  u_int16_t ip_len;                             /* Total length. */
  u_int16_t ip_id;                              /* Id. */
  short          ip_off;                             /* Fragment offset. */
  u_char ip_ttl;                              /* TTL. */
  u_char ip_p;                                /* Protocol. */
  u_int16_t ip_sum;                             /* Checksum. */
  hsl_ipv4Address_t ip_src;                          /* Source. */
  hsl_ipv4Address_t ip_dst;                          /* Destination. */
} __attribute__((__packed__));

#ifdef HAVE_IPV6
/*
  IPv6 header.
*/
struct hsl_ip6_hdr
{
  u_int32_t  ver_class_flow;  /* 4 bit version, 8 bit class, 20 bit flow. */
  u_int16_t  plen;            /* Payload length. */
  u_int8_t   nxt_hdr;         /* Next Header. */
  u_int8_t   hlim;            /* Hop limit. */

  hsl_ipv6Address_t ip_src;    /* IPv6 source address. */
  hsl_ipv6Address_t ip_dst;    /* IPv6 destination address. */
} __attribute__((__packed__));

/*
  ICMPv6 header.
*/
struct hsl_icmp6_hdr
{
  u_int8_t   icmp6_type;   /* type field */
  u_int8_t   icmp6_code;   /* code field */
  u_int16_t  icmp6_cksum;  /* checksum field */
  union 
  {
     u_int32_t icmp6_un_data32[1]; /* type-specific field */
     u_int16_t icmp6_un_data16[2]; /* type-specific field */
     u_int8_t  icmp6_un_data8[4];  /* type-specific field */
  } icmp6_dataun;
} __attribute__((__packed__));

/* ICMPv6 Neighbor Discovery Options Types */
struct hsl_nd_opt_hdr
{
   u_int8_t nd_opt_type;
   u_int8_t nd_opt_len;
   u_int8_t nd_opt_data[1];
}__attribute__((__packed__));

#endif /* HAVE_IPV6 */ 

#ifdef HAVE_VRRP
struct hsl_vrrp_header
{
#if defined(WORDS_LITTLEENDIAN)
  u_char version:4;                             /* Version. */
  u_char type:4;                                /* Type.    */
#elif defined(WORDS_BIGENDIAN)
  u_char version:4;
  u_char type:4;
#endif
  u_char virtual_rtr_id;                        /* Virtual RTR ID. */
  u_char priority;                              /* Priority. */
  u_char count_ip_addr;                         /* Count IP addr. */
  u_char auth_type;                             /* Auth Type.     */
  u_char advert_int;                            /* Advert Interval.*/
  u_int16_t checksum;                           /* Checksum.  */
  union
  {
    hsl_ipv4Address_t ip4_addr;
#ifdef HAVE_IPV6
    hsl_ipv6Address_t ip6_addr;
#endif
  } vip;                                      /* Virtual IP address. */
};
#endif /* HAVE_VRRP */

#define HSL_PROTO_IP           0   /* Dummy for IP. */
#define HSL_PROTO_ICMP         1   /* Internet Control Message Protocol. */
#define HSL_PROTO_IGMP         2   /* Internet Group Management Protocol. */
#define HSL_PROTO_IPIP         4   /* IPv4 inside IPv4. */
#define HSL_PROTO_IPV4         4
#define HSL_PROTO_TCP          6   /* Transmisstion Control Protocol */
#define HSL_PROTO_UDP          17  /* User Datagram Protocol */
#define HSL_PROTO_IPV6         41  /* IPv6 inside IPv4 */
#define HSL_PROTO_RSVP         46  /* Reservation Protocol */
#define HSL_PROTO_GRE          47  /* Cisco GRE tunnels (RFC 1701, 1702) */
#define HSL_PROTO_ESP          50  /* Encap. Security Payload. */
#define HSL_PROTO_AH           51  /* Authentication header. */
#define HSL_PROTO_ICMPV6       58  /* Internet Control Message Protocol version 6 */
#define HSL_PROTO_OSPFIGP      89  /* Open Shortest Path First protocol */
#define HSL_PROTO_PIM          103 /* Protocol Independent Multicast */
#define HSL_PROTO_RAW          255 /* Raw IP datagram. */
#define HSL_PROTO_MAX          256

#define HSL_L2_ETH_P_8021Q              0x8100
#define HSL_L2_ETH_P_8021Q_STAG         0x88a8
#define HSL_L2_ETH_P_CFM                0x8902

/*
  ARP header.
*/
struct hsl_arp
{
  u_int16_t hrd;                                /* Format of hardware address. */
  u_int16_t pro;                                /* Format of protocol address. */
  u_int16_t hln_pln;                            /* Length of hardware and protocol address. */
  u_int16_t op;                                 /* Code. */
  u_char eth_src[HSL_ETHER_ALEN]; 
  u_int16_t ip_src[2];
  u_char eth_dst[HSL_ETHER_ALEN];
  u_int16_t ip_dstp[2];
} __attribute__((__packed__));

#if defined (WORDS_LITTLEENDIAN)
#define GET_32ON16(addr) ((u_int32_t)(((u_int16_t *)(addr))[1] << 16 | ((u_int16_t *)(addr))[0]))
#elif defined (WORDS_BIGENDIAN)
#define GET_32ON16(addr) ((u_int32_t)(((u_int16_t *)(addr))[0] << 16 | ((u_int16_t *)(addr))[1]))
#else 
#define GET_32ON16(addr) ((u_int32_t)(((u_int16_t *)(addr))[1] << 16 | ((u_int16_t *)(addr))[0]))
#endif /* endian. */

/*
  ARP header values.
*/
#define HSL_ARP_HRD_ETHER                            htons(0x0001)
#define HSL_ARP_PRO_IP                               htons(HSL_ETHER_TYPE_IP)
#define HSL_ARP_HLN_PLN                              htons(0x0604)

#define HSL_ARP_OP_REQUEST                           htons(0x0001)
#define HSL_ARP_OP_REPLY                             htons(0x0002)

/* 
   Max bit/byte length of IPv4 address. 
*/
#define IPV4_MAX_BYTELEN    4
#define IPV4_MAX_BITLEN    32
#define IPV4_MAX_PREFIXLEN 32

#ifndef HAVE_USER_HSL
#define IPV4_ADDR_CMP(D,S)   (memcmp ((D), (S), IPV4_MAX_BYTELEN))
#define IPV4_ADDR_SAME(D,S)  (memcmp ((D), (S), IPV4_MAX_BYTELEN) == 0)
#define IPV4_ADDR_COPY(D,S)  (memcpy ((D), (S), IPV4_MAX_BYTELEN))
#endif /* HAVE_USER_HSL */

#define IPV4_ADDR_MULTICAST(addr) IN_CLASSD(addr)
#define IPV4_ADDR_MARTIAN(X)                                          \
  ((IN_CLASSA (X)                                                     \
    && ((((u_int32_t) (X)) & IN_CLASSA_NET) == 0x00000000L            \
        || (((u_int32_t) (X)) & IN_CLASSA_NET) == 0x7F000000L))       \
   || (IN_CLASSB (X)                                                  \
       && ((((u_int32_t) (X)) & IN_CLASSB_NET) == 0x80000000L         \
           || ((((u_int32_t) (X))) & IN_CLASSB_NET) == 0xBFFF0000L))  \
   || (IN_CLASSC (X)                                                  \
       && ((((u_int32_t) (X)) & IN_CLASSC_NET) == 0xC0000000L         \
           || ((((u_int32_t) (X))) & IN_CLASSC_NET) == 0xDFFFFF00L)))

#define IPV4_NET0(a)    ((((u_int32_t) (a)) & 0xff000000) == 0x00000000)
#define IPV4_NET127(a)  ((((u_int32_t) (a)) & 0xff000000) == 0x7f000000)

#define CLASS_A_BROADCAST(a)   ((((u_int32_t) (a)) & 0x00ffffff) == 0x00ffffff)
#define CLASS_B_BROADCAST(a)   ((((u_int32_t) (a)) & 0x0000ffff) == 0x0000ffff)
#define CLASS_C_BROADCAST(a)   ((((u_int32_t) (a)) & 0x000000ff) == 0x000000ff)

#ifndef INADDR_MAX_LOCAL_GROUP
#define INADDR_MAX_LOCAL_GROUP 0xe00000ffU
#endif /* INADDR_MAX_LOCAL_GROUP */

#ifdef HAVE_IPV6

/* Neighbor Discovery ICMPv6 Types. */
#define IPV6_ND_OPT_TARGET_LINKADDR      2
#define IPV6_ND_OPT_SRC_LINKADDR         1
#define IPV6_ND_ROUTER_SOLICIT         133
#define IPV6_ND_ROUTER_ADVERT          134
#define IPV6_ND_NEIGHBOR_SOLICIT       135
#define IPV6_ND_NEIGHBOR_ADVERT        136
#define IPV6_ND_REDIRECT               137

/* Max bit/byte length of IPv6 address. */
#define IPV6_MAX_BYTELEN    16
#define IPV6_MAX_BITLEN    128
#define IPV6_MAX_PREFIXLEN 128
#ifndef HAVE_USER_HSL
#define IPV6_ADDR_CMP(D,S)   (memcmp ((D), (S), IPV6_MAX_BYTELEN))
#define IPV6_ADDR_SAME(D,S)  (memcmp ((D), (S), IPV6_MAX_BYTELEN) == 0)
#define IPV6_ADDR_COPY(D,S)  (memcpy ((D), (S), IPV6_MAX_BYTELEN))
#endif /* HAVE_USER_HSL */
#define IPV6_IS_ADDR_MULTICAST(addr)   (((unsigned char *)(addr))[0] == 0xff) 
#define IPV6_IS_ADDR_MC_LINKLOCAL(addr)   (IPV6_IS_ADDR_MULTICAST(addr) && \
    ((((unsigned char *)(addr))[1] & 0xf) == 0x2)) 
#define IPV6_IS_ADDR_LINKLOCAL(addr)   ((((unsigned int *)(addr))[0] & htonl (0xffc00000)) == htonl (0xfe800000)) 
#define HSL_IPV6_ADDR_ZERO(addr) (((addr).in6_u.u6_addr32[0] == 0) && ((addr).in6_u.u6_addr32[1] == 0) &&        \
                                  ((addr).in6_u.u6_addr32[2] == 0) && ((addr).in6_u.u6_addr32[3] == 0))

#define HSL_IPV6_ZERO_ADDR(addr) (((addr).word[0] == 0) && ((addr).word[1] == 0) &&  \
                                  ((addr).word[2] == 0) && ((addr).word[3] == 0))
#ifdef PNE_VERSION_2_2
#define IPV6_IS_ADDR_LOOPBACK(a) \
        ((((u_int32_t *) (a))[0] == 0                                 \
         && ((u_int32_t *) (a))[1] == 0                               \
         && ((u_int32_t *) (a))[2] == 0                               \
         && ((u_int32_t *) (a))[3] == htonl (1)) ||                   \
         (((u_int32_t *) (a))[0] == htonl(0xfe800000)                 \
         && ((u_int32_t *) (a))[1] == 0                               \
         && ((u_int32_t *) (a))[2] == 0                               \
         && ((u_int32_t *) (a))[3] == htonl (1)))
#else /* PNE_VERSION_2_2 */
#define IPV6_IS_ADDR_LOOPBACK(a) \
        (((u_int32_t *) (a))[0] == 0                                  \
         && ((u_int32_t *) (a))[1] == 0                               \
         && ((u_int32_t *) (a))[2] == 0                               \
         && ((u_int32_t *) (a))[3] == htonl (1)) 
#endif /* PNE_VERSION_2_2 */

#endif /* HAVE_IPV6 */

#ifdef HAVE_IPV6

#define HSL_PREFIX_CMP(D,S)                                             \
           ((AF_INET == ((D)->family)) && (AF_INET == ((S)->family)))?  \
               IPV4_ADDR_CMP(&((D)->u.prefix4),&((S)->u.prefix4)):      \
           ((AF_INET6 == ((D)->family)) && (AF_INET6 == ((S)->family)))?\
               IPV6_ADDR_CMP(&((D)->u.prefix6),&((S)->u.prefix6)):0;

#define HSL_PREFIX_IS_ZERO(D)                                                   \
           ((AF_INET == ((D)->family))? (((D)->u.prefix4)? HSL_FALSE: HSL_TRUE):\
            ((AF_INET6 == ((D)->family))?                                       \
                (HSL_IPV6_ZERO_ADDR((D)->u.prefix6)?HSL_TRUE:HSL_FALSE):0))


#ifdef PNE_VERSION_2_2
#define IPV6_LINKLOCAL_IFINDEX(addr)  ((addr).s6_addr[2] << 8 | (addr).s6_addr[3])
#define IPV6_SET_IN6_LINKLOCAL_IFINDEX(addr, ifindex)    \
  do {                                                   \
    (addr).s6_addr[2] = ((ifindex) >> 8) & 0xff;         \
    (addr).s6_addr[3] = (ifindex) & 0xff;                \
  } while (0)
#endif /*  PNE_VERSION_2_2 */

#else   /* HAVE_IPV6 */

#define HSL_PREFIX_CMP(D,S)                                             \
           ((AF_INET == ((D)->family)) && (AF_INET == ((S)->family)))?  \
               IPV4_ADDR_CMP(&((D)->u.prefix4),&((S)->u.prefix4)):0;

#define HSL_PREFIX_IS_ZERO(D)                                                   \
           ((AF_INET == ((D)->family))? (((D)->u.prefix4)? HSL_FALSE: HSL_TRUE): 0)

#endif  /* HAVE_IPV6 */

#ifndef HAVE_USER_HSL

/* Socket address structure for L2 */
struct sockaddr_l2
{
  /* Destination Mac address */
  unsigned char dest_mac[HSL_ETHER_ALEN];

  /* Source Mac address */
  unsigned char src_mac[HSL_ETHER_ALEN];

  /* Outgoing/Incoming interface index */
  unsigned int port;
};

/* Socket address structure for tagged L2 control packets. */
struct sockaddr_vlan
{
  /* Destination Mac address */
  unsigned char dest_mac[HSL_ETHER_ALEN];

  /* Source Mac address */
  unsigned char src_mac[HSL_ETHER_ALEN];

  /* Outgoing/Incoming interface index */
  unsigned int port;

  /* Vlan id */
  unsigned short vlanid;

  /* SVlan id */
  unsigned short svlanid;

  /* Length of the Packet */
  unsigned int length;
};


#ifdef HAVE_MLD_SNOOP

#define HSL_MLD_HOP_LIMIT_DEF 1

/* Socket address structure for tagged MLD control packets. */
struct sockaddr_mld
{
  /* Destination Mac address */
  unsigned char dest_mac[HSL_ETHER_ALEN];

  /* Source Mac address */
  unsigned char src_mac[HSL_ETHER_ALEN];

  /* Outgoing/Incoming interface index */
  unsigned int port;

  /* Vlan id */
  unsigned short vlanid;

  hsl_ipv6Address_t ip6_src;

  hsl_ipv6Address_t ip6_dst;
};

#endif /* HAVE_MLD_SNOOP */
#endif /* HAVE_USER_HSL */


/* General */
typedef enum
{
  HSL_FALSE = 0,
  HSL_TRUE = 1
}HSL_BOOL;

#define HSL_DSCP_TBL_SIZE 64

#define HSL_SET_HTONL(xxaddr, yyval) do { \
   ((u_char *)(xxaddr))[0] = (u_char)((((u_int32_t)(yyval)) >> 24) & 0xff); \
   ((u_char *)(xxaddr))[1] = (u_char)((((u_int32_t)(yyval)) >> 16) & 0xff); \
   ((u_char *)(xxaddr))[2] = (u_char)((((u_int32_t)(yyval)) >>  8) & 0xff); \
   ((u_char *)(xxaddr))[3] = (u_char) (((u_int32_t)(yyval)) & 0xff); \
} while(0)
#define HSL_SET_NTOHL(xxaddr, yyval) HSL_SET_HTONL(xxaddr,yyval)

#define HSL_GET_HTONL(xxaddr) \
   ((u_int32_t)(((u_char *)(xxaddr))[0] << 24) + \
    (u_int32_t)(((u_char *)(xxaddr))[1] << 16) + \
    (u_int32_t)(((u_char *)(xxaddr))[2] << 8)  + \
    (u_int32_t) ((u_char *)(xxaddr))[3])
#define HSL_GET_NTOHL(xxaddr)  HSL_GET_HTONL(xxaddr)

#define HSL_MAC_IS_ZERO(m)   \
((m)[0] == 0x00 &&           \
 (m)[1] == 0x00 &&           \
 (m)[2] == 0x00 &&           \
 (m)[3] == 0x00 &&           \
 (m)[4] == 0x00 &&           \
 (m)[5] == 0x00)

#define HSL_MAC_IS_ALL_ONES(m)   \
((m)[0] == 0xff &&               \
 (m)[1] == 0xff &&               \
 (m)[2] == 0xff &&               \
 (m)[3] == 0xff &&               \
 (m)[4] == 0xff &&               \
 (m)[5] == 0xff)

#define HSL_MAC_IS_INVALID(m) \
(HSL_MAC_IS_ZERO(m) || HSL_MAC_IS_ALL_ONES(m)) 

#define HSL_MAC_CMP(m1,m2)     (memcmp((m1),(m2),HSL_ETHER_ALEN))
#define HSL_MAC_IS_SAME(m1,m2) (HSL_MAC_CMP((m1), (m2)) == 0)
/*
  Function prototypes. 
*/
int hsl_prefix_same (hsl_prefix_t *p1, hsl_prefix_t *p2);
void hsl_masklen2ip (u_int32_t masklen, hsl_ipv4Address_t *netmask);
void hsl_apply_mask_ipv4 (hsl_prefix_t *p);

#ifdef HAVE_IPV6
void hsl_masklen2ip6 (s_int32_t masklen, hsl_ipv6Address_t *netmask);
void hsl_apply_mask_ipv6 (hsl_prefix_t *p);
#endif /* HAVE_IPV6 */
void hsl_prefix_copy (hsl_prefix_t *dest, hsl_prefix_t *src);
int hsl_prefix_match (hsl_prefix_t *n, hsl_prefix_t *p);
int hsl_prefix2str (hsl_prefix_t *p, char *str, int size);
char *hsl_inet_ntop (int af, void *src, char *dst, int size);
int hsl_inet_pton (int af, const char *strptr, void *addrptr);
int hsl_strlen(char *s);
void hsl_add_uint64(ut_int64_t *a,ut_int64_t *b, ut_int64_t *res);
void hsl_sub_uint64(ut_int64_t *a,ut_int64_t *b, ut_int64_t *res);
void hsl_apply_mask (hsl_prefix_t *);
char *hsl_mac_2_str(u_char *mac,char *dst, size_t size);
void *hsl_malloc(u_int32_t size);
void hsl_free(void *ptr);

#endif /* _HSL_TYPES_H_ */

