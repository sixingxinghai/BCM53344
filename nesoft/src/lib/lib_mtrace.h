/* Copyright (C) 2005  All Rights Reserved. */

#ifndef _PACOS_LIB_MTRACE_H
#define _PACOS_LIB_MTRACE_H

/* mtrace response routing protocol types */
enum
{
  MTRACE_PROTO_DVMRP = 1,
  MTRACE_PROTO_MOSPF,
  MTRACE_PROTO_PIM,
  MTRACE_PROTO_CBT,
  MTRACE_PROTO_PIM_SPCL,
  MTRACE_PROTO_PIM_STATIC,
  MTRACE_PROTO_DVMRP_STATIC,
  MTRACE_PROTO_PIM_MBGP,
  MTRACE_PROTO_CBT_SPCL,
  MTRACE_PROTO_CBT_STATIC,
  MTRACE_PROTO_PIM_ASSERT
};

/* mtrace response forwarding codes */
enum
{
  MTRACE_CODE_NO_ERR         =    0x00,
  MTRACE_CODE_WRONG_IF       =    0x01,
  MTRACE_CODE_PRUNE_SENT     =    0x02,
  MTRACE_CODE_PRUNE_RCVD     =    0x03,
  MTRACE_CODE_SCOPED         =    0x04,
  MTRACE_CODE_NO_ROUTE       =    0x05,
  MTRACE_CODE_WRONG_LAST_HOP =    0x06,
  MTRACE_CODE_NOT_FWDING     =    0x07,
  MTRACE_CODE_REACHED_RP     =    0x08,
  MTRACE_CODE_RPF_IF         =    0x09,
  MTRACE_CODE_NO_MULTICAST   =    0x0A,
  MTRACE_CODE_INFO_HIDDEN    =    0x0B,
  MTRACE_CODE_FATAL_ERROR    =    0x80,
  MTRACE_CODE_NO_SPACE       =    0x81,
  MTRACE_CODE_OLD_ROUTER     =    0x82,
  MTRACE_CODE_ADMIN_PROHIB   =    0x83,
};

#define MTRACE_SET_FWD_CODE(opaque, v, fwd_code, dbg_func)                  \
  do                                                                        \
  {                                                                         \
    u_int8_t __old_code = (v);                                              \
                                                                            \
    if ((v) == MTRACE_CODE_NO_ERR)                                          \
      v = (fwd_code);                                                       \
                                                                            \
    if (dbg_func != NULL)                                                   \
      (*(dbg_func)) ((opaque), __old_code, (v));                            \
                                                                            \
  } while(0)

#define MTRACE_SET_FWD_CODE_OVERRIDE(opaque, v, fwd_code, dbg_func)         \
  do                                                                        \
  {                                                                         \
    u_int8_t __old_code = (v);                                              \
                                                                            \
    v = (fwd_code);                                                         \
                                                                            \
    if (dbg_func != NULL)                                                   \
      (*(dbg_func)) ((opaque), __old_code, (v));                            \
                                                                            \
  } while(0)

/* Set the forwarding code in last response block if there is one */
#define MTRACE_SET_LAST_RESP_FWD_CODE(opaque, pkt, pkt_len, code, dbg_func)\
  do                                                                       \
  {                                                                        \
    u_char *_p;                                                            \
    if ((pkt_len) <= MTRACE_QUERY_SIZE)                                    \
      break;                                                               \
    _p = (u_char *)(pkt) + (pkt_len) - 1;                                  \
    MTRACE_SET_FWD_CODE_OVERRIDE ((opaque), *_p, (code), (dbg_func));      \
  } while (0)

/* mtrace packet structures */

#define MTRACE_QUERY_REQUEST          0x1F
#define MTRACE_RESPONSE               0x1E

#define MTRACE_PKT_TYPE_STR(t,q)                                          \
  (((t) == MTRACE_RESPONSE) ? "Response" :                                \
  (((t) == MTRACE_QUERY_REQUEST) ? ((q) ? "Query" : "Request") :          \
   "Unknown"))

/* Portion of mtrace header corresponding to IGMP header */
struct mtrace_hdr_igmp
{
  u_int8_t            igmp_type;            /* IGMP type */
  u_int8_t            num_hops;             /* Number of hops requested */
  u_int16_t           checksum;             /* Checksum of mtrace packet */
  struct pal_in4_addr group;                /* Multicast group address */
};
#define MTRACE_IGMPHDR_SIZE      sizeof (struct mtrace_hdr_igmp)

/* mtrace header */
struct mtrace_hdr
{
  struct mtrace_hdr_igmp           mt_igmp;      /* IGMP header */
  struct pal_in4_addr              mt_source;    /* Multicast source address */
  struct pal_in4_addr              mt_dest;      /* Multicast receiver address */
  struct pal_in4_addr              mt_resp;      /* Response address */
  u_int32_t                        mt_ttl_qid;   /* Resp TTL + Query Id */
};
#define mt_igmp_type    mt_igmp.igmp_type
#define mt_num_hops     mt_igmp.num_hops
#define mt_checksum     mt_igmp.checksum
#define mt_group        mt_igmp.group

#define MTRACE_HDR_SIZE         sizeof (struct mtrace_hdr)
#define MTRACE_QUERY_SIZE       MTRACE_HDR_SIZE

#define MTRACE_HDR_GET_QUERY_ID(v)  ((v) & 0x00FFFFFF)
#define MTRACE_HDR_GET_RESP_TTL(v)  (u_int8_t)(((v) & 0xFF000000) >> 24)

/* mtrace response */
struct mtrace_resp
{
  u_int32_t           query_arr_t;          /* Query Arrival Time */
  struct pal_in4_addr iif_addr;             /* Incoming Interface address */
  struct pal_in4_addr oif_addr;             /* Outgoing Interface address */
  struct pal_in4_addr phop_addr;            /* Previous Hop address */
  u_int32_t           iif_ipkt;             /* Iif Input packet count */
  u_int32_t           oif_opkt;             /* Oif Output packet count */
  u_int32_t           sg_pkt;               /* (S,G) packet count */
  u_int8_t            rtg_proto;            /* Routing protocol type */
  u_int8_t            fwd_ttl;              /* Minimum TTL on Oif */
  u_int8_t            src_mask;             /* S-bit + Source mask length */
  u_int8_t            fwd_code;             /* Forwarding code */
};

#define MTRACE_RESP_SIZE         sizeof (struct mtrace_resp)

#define MTRACE_RESP_SET_S_BIT(v)          ((v) |= 0x40)
#define MTRACE_RESP_UNSET_S_BIT(v)        ((v) &= ~(0x40))
#define MTRACE_RESP_SET_SRC_MASK(v,m)     ((v) |= ((m) & 0x3F))

#define MTRACE_RESP_GET_S_BIT(v)          (((v) & 0x40) >> 6)
#define MTRACE_RESP_GET_SRC_MASK(v)       ((v) & 0x3F)

#define MTRACE_IS_ADDR_VALID(a) ((a)->s_addr != 0xFFFFFFFF)

#define MTRACE_INVALID_PKT_CNT  (0xFFFFFFFF)

#define MTRACE_GRP_MASK     63


char *mtrace_rtg_proto_str (u_int8_t);
char *mtrace_fwd_code_str (u_int8_t);
#endif /* _PACOS_LIB_MTRACE_H */
