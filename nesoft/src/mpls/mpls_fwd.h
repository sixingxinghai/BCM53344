/* Copyright (C) 2001-2003  All Rights Reserved.  */

#ifndef _PACOS_MPLS_FWD_H
#define _PACOS_MPLS_FWD_H

/* The necessary header files */
#include <linux/init.h>
#include <linux/autoconf.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/proc_fs.h>
#include <linux/ip.h>
#include <net/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/icmp.h>
#include <linux/in.h>
#include <net/ip_fib.h>
#include <net/net_namespace.h>
#include <asm/uaccess.h>
#include <linux/inetdevice.h>
#include <linux/igmp.h>
#include <linux/if_arp.h>
#include <net/icmp.h>
#include <net/route.h>

/* UNPALORIZE !! */
#undef pal_in4_addr
#define pal_in4_addr in_addr

/* Deal with CONFIG_MODVERSIONS */
#if defined (CONFIG_MODVERSIONS)
#define MODVERSIONS
/* #include <linux/modversions.h> */
#endif

/* PRINTK macros (Number corresponds to argument #) */
#define PRINTK_NOTICE(x...) \
  do { \
    if (show_notice_msg == 1) \
      printk (KERN_NOTICE mpls_module_name x); \
  } while (0)

#define PRINTK_DEBUG(x...) \
  do { \
    if (show_debug_msg == 1) \
      printk (KERN_NOTICE mpls_module_name x); \
    else \
      printk (KERN_DEBUG mpls_module_name x); \
  } while (0)

#define PRINTK_WARNING(x...) \
  do { \
    if (show_warning_msg == 1) \
      printk (KERN_WARNING mpls_module_name x); \
  } while (0)

#define PRINTK_ERR(x...) \
  do { \
    if (show_error_msg == 1) \
      printk (KERN_ERR mpls_module_name x); \
  } while (0)

/*---------------------------------------------------------------------------
  typedefs to save us some keystrokes.
  --------------------------------------------------------------------------*/
typedef unsigned char           uint8;
typedef signed char             sint8;
typedef unsigned short int      uint16;
typedef signed short int        sint16;
typedef unsigned long int       uint32;
typedef unsigned long long int  uint64;
typedef signed long int         sint32;

typedef uint32             LABEL_TYPE;
typedef sint32             LABEL;
typedef uint32             L3ADDR;

#define ETH_P_MPLS_UC          0x8847 /* MPLS Unicast traffic   */
#define ETH_P_MPLS_MC          0x8848 /* MPLS Multicast traffic */

#define mpls_module_name "mpls : "

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#ifdef HAVE_DEV_TEST
#define MPLS_ASSERT(x) \
   if (!(x)) \
    { \
       PRINTK_DEBUG ("%s:%d assert failed for %s\n", \
               __FILE__, __LINE__, (#x)); \
    }
#else
#define MPLS_ASSERT(x)
#endif /* HAVE_DEV_TEST  */

#define LOOPBACK(a) IN_LOOPBACK(a)

/* This defines the module identifier.  Copied from
   pal/api/pal_types.def.  */
typedef enum
{
  APN_PROTO_UNSPEC,             /* Unspecified module.  */
  APN_PROTO_NSM,                /* NSM */
  APN_PROTO_RIP,                /* RIP (IPv4) */
  APN_PROTO_RIPNG,              /* RIPng (IPv6) */
  APN_PROTO_OSPF,               /* OSPF (IPv4) */
  APN_PROTO_OSPF6,              /* OSPFv3 (IPv6) */
  APN_PROTO_BGP,                /* BGP (IPv4/IPv6) */
  APN_PROTO_LDP,                /* LDP */
  APN_PROTO_RSVP,               /* RSVP */
  APN_PROTO_ISIS,               /* IS-IS */
  APN_PROTO_PIMDM,              /* PIM-DM */
  APN_PROTO_PIMSM,              /* PIM-SM */
  APN_PROTO_VTYSH,              /* VTYSH */
  APN_PROTO_PIMPKTGEN,          /* PIM packet generator */
  APN_PROTO_OAM,
  APN_PROTO_MAX                 /* No module.  Must be last.  */
} module_id_t;

/*
 * this is how we identify our interface address
 */
#define is_myaddr(net,addr) (inet_addr_type(net,addr) == RTN_LOCAL)


/* Forwarding types */
#define MPLS_VPN_FWD    1
#define MPLS_LABEL_FWD  2
#define MPLS_IP_FWD     3
#define LOCAL_IP_FWD    4
#define MPLS_VC_FWD     5

/*---------------------------------------------------------------------------
  Generic MPLS shim header.

  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ Label
  |                Label                  | Exp |S|       TTL     | Stack
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ Entry
  --------------------------------------------------------------------------*/

struct shimhdr
{
  u32            shim;

  /*-----------------------------------------------------------------------
    Some hash defines to get the different fields of the shim header.
    -----------------------------------------------------------------------*/

  /*
   * *_host macros are used with the first argument (only argument in case of
   * get macros) in host byte order.
   * *_net macros are used with the first argument in network byte order.
   * Second argument for the set macros is always in host byte order.
   */
#if defined (__BIG_ENDIAN)                        /* Big Endian architecture */

#define get_label_net(shim)      (((shim) >> 12) & 0xFFFFF)
#define get_exp_net(shim)        (((shim) >> 9)  & 0x7)
#define get_bos_net(shim)        (((shim) >> 8)  & 0x1)
#define get_ttl_net(shim)        ((shim)  & 0xFF)
#define set_label_net(shim, val) {(shim) &= 0x00000FFF;(shim) |= (val)<<12;}
#define set_exp_net(shim, val)   \
                                 {MPLS_ASSERT((val) >= 0 & (val) < 8);\
                                 (shim) &= 0xFFFFF1FF;(shim) |= (val)<<9;}
#define set_bos_net(shim, val)   \
                                 {MPLS_ASSERT((val) == 1 || (val) == 0);\
                                 (shim) &= 0xFFFFFEFF;(shim) |= (val)<<8;}
#define set_ttl_net(shim, val)   \
                                 {MPLS_ASSERT((val) >= 0 && (val) < 256);\
                                 (shim) &= 0xFFFFFF00;(shim) |= (val);}
#define get_label_host(shim)     get_label_net(shim)
#define get_exp_host(shim)       get_exp_net(shim)
#define get_bos_host(shim)       get_bos_net(shim)
#define get_ttl_host(shim)       get_ttl_net(shim)
#define set_label_host(shim,val) set_label_net(shim,val)
#define set_exp_host(shim,val)   set_exp_net(shim,val)
#define set_bos_host(shim,val)   set_bos_net(shim,val)
#define set_ttl_host(shim,val)   set_ttl_net(shim,val)

#elif defined (__LITTLE_ENDIAN)                    /* Little Endian architecture */

#define get_label_net(shim)      (u32)(\
                                   (((((uint8*)&(shim))[2]>>4)&0x0F) | \
                                   ((((uint8*)&(shim))[1])<<4) | \
                                   (((uint8*)&(shim))[0])<<12)&0x000FFFFF\
                                 )
#define get_exp_net(shim)        (((((uint8*)&(shim))[2])>>1) & 0x07)
#define get_bos_net(shim)        ((((uint8*)&(shim))[2]) & 0x01)
#define get_ttl_net(shim)        (((uint8*)&(shim))[3])
#define set_label_net(shim,val)  (((uint8*)&(shim))[2]) &= 0x0F;\
                                 (((uint8*)&(shim))[1]) &= 0x00;\
                                 (((uint8*)&(shim))[0]) &= 0x00;\
                                 (((uint8*)&(shim))[2]) |= ((val)&0x0F)<<4;\
                                 (((uint8*)&(shim))[1]) = ((val)>>4)&0xFF;\
                                 (((uint8*)&(shim))[0]) = ((val)>>12)&0xFF;
#define set_exp_net(shim, val)   (((uint8*)&(shim))[2]) &= 0xF1;\
                                 (((uint8*)&(shim))[2]) |= ((val)<<1 & 0x0E);
#define set_bos_net(shim, val)   (((uint8*)&(shim))[2]) &= 0xFE;\
                                 (((uint8*)&(shim))[2]) |= ((val) & 0x01);
#define set_ttl_net(shim, val)   (((uint8*)&(shim))[3]) &= 0x00;\
                                 (((uint8*)&(shim))[3]) = (val)        ;

#define get_label_host(shim)     (((shim) >> 12) & 0xFFFFF)
#define get_exp_host(shim)       (((shim) >> 9)  & 0x7)
#define get_bos_host(shim)       (((shim) >> 8)  & 0x1)
#define get_ttl_host(shim)       ((shim)  & 0xFF)
#define set_label_host(shim, val) {(shim) &= 0x00000FFF;(shim) |= (val)<<12;}
#define set_exp_host(shim, val)   \
                                  {MPLS_ASSERT((val) >= 0 & (val) < 8);\
                                  (shim) &= 0xFFFFF1FF;(shim) |= (val)<<9;}
#define set_bos_host(shim, val)   \
                                  {MPLS_ASSERT((val) == 1 || (val) == 0);\
                                  (shim) &= 0xFFFFFEFF;(shim) |= (val)<<8;}
#define set_ttl_host(shim, val)   \
                                  {MPLS_ASSERT((val) >= 0 && (val) < 256);\
                                  (shim) &= 0xFFFFFF00;(shim) |= (val);}

#else
#error check your byteorder definitions.
#endif
};

/**@brief - Structure for the PW ACH Header which is of size 4 bytes.
 * Also the utility macros to get/set the values in each field are written.
 */
struct pwachhdr
{
  u_int32_t            pwach;

#if defined (__BIG_ENDIAN)                        /* Big Endian architecture */
#define get_first_nibble_net(pwach)      (((pwach) >> 28) & 0xFFFFF)
#define get_channel_type_net(pwach)        (((pwach))  & 0x10000)
#define set_first_nibble_net(pwach, val) {(pwach) &= 0x0FFFFFFF;(pwach) |= (val)<<28;}
#define set_channel_type_net(pwach, val) {(pwach) &= 0xFFFF0000;(pwach) |= (val);}
#elif defined (__LITTLE_ENDIAN)                    /* Little Endian architecture */
#define set_first_nibble_net(pwach,val)   (((uint8*)&(pwach))[0]) &= 0x00;\
                                          (((uint8*)&(pwach))[1]) &= 0x00;\
                                          (((uint8*)&(pwach))[0]) = (1<<4)&0xFF;
#define set_channel_type_net(pwach, val)  (((uint8*)&(pwach))[2]) &= 0x00;\
                                          (((uint8*)&(pwach))[3]) &= 0x00;\
                                          (((uint8*)&(pwach))[3]) = (val);\
                                          (((uint8*)&(pwach))[2]) = (val << 8);
#define get_channel_type_net(pwach)       ((((uint16*)&(pwach))[1] & 0xFFFF))
#else
#error check your byteorder definitions.
#endif
};

/*
 * since Linux does'nt know about MPLS, it initializes the layer 3 protocol
 * pointer incorrectly for labelled incoming packets, this macro corrects
 * that
 */
#define set_proto_pointers(skb)\
{\
    switch(ntohs(((struct ethhdr *)(skb->mac_header))->h_proto))\
    {\
        case ETH_P_IP:\
           /*\
            * we do not support IPv6.\
            */\
           if(ip_hdr(skb)->version == IPV6)\
           {\
               PRINTK_WARNING ("IPv6 not supported.\n");\
               return(0);\
           }\
        break;\
        case ETH_P_MPLS_UC:\
        {\
             u32 j;\
             struct shimhdr *shimhp;\
             PRINTK_DEBUG ("MPLS unicast packet received\n");\
             shimhp = (struct shimhdr *)(skb->mac_header + ETH_HLEN);  \
/*---------------------------------------------------------------------------\
  Since we can have multiple shim headers (label stacking) we have to \
  scan all till we hit one with the BOS bit set. This signifies the end\
  of label stack and hence the starting of the network layer header.\
  ---------------------------------------------------------------------------*/ \
             for(j=0;;j+=SHIM_HLEN)\
             {\
                 if(get_bos_net(*((u32*)((uint8*)shimhp+j))))\
                 {\
                     break;\
                 }\
             }\
           /* If PW ACH Header is present, network header will be SHIM_HLEN\
            * bytes further */\
           if(IS_PW_ACH(*(((uint8*)shimhp)+j+SHIM_HLEN)))\
             j += PW_ACH_LEN;\
           /* Adjust the network header */\
           skb->network_header  = \
             (sk_buff_data_t)((uint8 *)shimhp + j + SHIM_HLEN);\
        }\
        break;\
        default:\
            /*\
             * if we get anything other then plain IP and MPLS unicast it \
             * is a serious problem as we have not registered for anything\
             * else\
             */\
             PRINTK_ERR ("PANIC, Received frame with type 0x%X\n",\
                        ntohs(((struct ethhdr *)(skb->mac_header))->h_proto));\
             break;\
    }\
}

/* Maximum initially labelled IP datagram size.  */
extern u32 milds;

/* the router alert label */
extern u32 router_alert;

/* Max fragment size allowable for the current sk_buff being
   processed */
extern u32 max_frag_size;

/* TTL to be used at ingress if modify_ttl_at_ingress is set */
extern int ingress_ttl;

/* TTL to be used at egress if modify_ttl_at_egress is set */
extern int egress_ttl;

/* TTL propagation behavior for INGRESS LER */
extern uint8 propagate_ttl;

/* Configured LSP Model */
extern uint8 lsp_model;

/* Flag to enable/disable handling of locally generated TCP packets */
extern uint8 local_pkt_handle;

/* Flag to indicate whether information should be printed or not */
extern uint8 show_error_msg;
extern uint8 show_warning_msg;
extern uint8 show_debug_msg;
extern uint8 show_notice_msg;

/* Opcodes for setting global variables. */
#define SET_DST_ALL         (1 << 0)
#define SET_DST_INPUT       (1 << 1)
#define SET_DST_OPS         (1 << 2)

/* Input/Output defines. */
#define RT_LOOKUP_INPUT     1
#define RT_LOOKUP_OUTPUT    2

#endif /* _PACOS_MPLS_FWD_H */
