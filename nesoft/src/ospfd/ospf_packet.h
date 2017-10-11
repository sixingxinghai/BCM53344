/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_OSPF_PACKET_H
#define _PACOS_OSPF_PACKET_H

#define IP_HEADER_SIZE              20
#define OSPF_HEADER_SIZE            24
#define OSPF_AUTH_SIMPLE_SIZE        8
#define OSPF_AUTH_MD5_SIZE          16

#define OSPF_PACKET_MIN_SIZE        64   /* Minimum packet buffer size.  */
#define OSPF_PACKET_MAX_SIZE     65535   /* Maximum packet buffer size.  */
#define OSPF_HELLO_MIN_SIZE         20   /* Not including neighbors.  */
#define OSPF_DB_DESC_MIN_SIZE        8
#define OSPF_LS_REQ_MIN_SIZE         0
#define OSPF_LS_UPD_MIN_SIZE         4
#define OSPF_LS_ACK_MIN_SIZE         0

#define OSPF_LS_REQ_KEY_SIZE        12

#define OSPF_HELLO_DEFAULT           1
#define OSPF_HELLO_RESET_NBR         2

#define OSPF_BUFFER_SIZE_DEFAULT  2048

/* This must be short, (less than RxmtInterval) 
   - RFC 2328 Section 13.5 para 3.  Set to 1 second to avoid Acks being
   held back for too long - MAG */
#define OSPF_DELAYED_ACK_DELAY       1

struct ospf_packet
{
  struct ospf_packet *next;
  struct ospf_packet *prev;

  /* Flags.  */
  u_char flags;
#define OSPF_PACKET_FLAG_MD5            (1 << 0)
#define OSPF_PACKET_FLAG_LLS            (1 << 1)

  /* OSPF packet type.  */
  u_char type;

  /* Dummy for alignment.  */
  u_int16_t dummy;

  /* Size of buffer allocated.  */
  u_int16_t alloced;

  /* OSPF packet length.  */
  u_int16_t length;

  /* IP destination address.  */
  struct pal_in4_addr dst;

  /* Pointer to data stream.  */
  u_char buf[OSPF_HEADER_SIZE];
};

/* OSPF packet header structure. */
struct ospf_header
{
  u_char version;                       /* OSPF Version. */
  u_char type;                          /* Packet Type. */
  u_int16_t length;                     /* Packet Length. */
  struct pal_in4_addr router_id;        /* Router ID. */
  struct pal_in4_addr area_id;          /* Area ID. */
  u_int16_t checksum;                   /* Check Sum. */
#ifdef HAVE_OSPF_MULTI_INST
  u_int8_t instance_id;                 /* Interface instance ID */
  u_int8_t auth_type;                   /* Authentication Type. */
#else
  u_int16_t auth_type;                  /* Authentication Type. */
#endif /* HAVE_OSPF_MULTI_INST */
  /* Authentication Data. */
  union
  {
    /* Simple Authentication. */
    char auth_data [OSPF_AUTH_SIMPLE_SIZE];
    /* Cryptographic Authentication. */
    struct
    {
      u_int16_t zero;                   /* Should be 0. */
      u_char key_id;                    /* Key ID. */
      u_char auth_data_len;             /* Auth Data Length. */
      u_int32_t crypt_seqnum;           /* Cryptographic Sequence Number. */
    } crypt;
  } u;
};

/* OSPF Hello body format. */
struct ospf_hello
{
  struct pal_in4_addr network_mask;
  u_int16_t hello_interval;
  u_char options;
  u_char priority;
  u_int32_t dead_interval;
  struct pal_in4_addr d_router;
  struct pal_in4_addr bd_router;
  struct pal_in4_addr neighbors[1];
};

/* OSPF Database Description. */
struct ospf_db_desc
{
  /* Interface MTU. */
  u_int16_t mtu;

  /* Options. */
  u_char options;

  /* Flags. */
  u_char flags;

  /* DD Sequence Number. */
  u_int32_t seqnum;
};

/* Macros. */
#ifdef HAVE_OSPF_MULTI_AREA
#define OSPF_PACKET_DST_HELLO(I)                                             \
    (((I)->type == OSPF_IFTYPE_VIRTUALLINK                                   \
      || (CHECK_FLAG ((I)->flags, OSPF_MULTI_AREA_IF)                        \
      && (I)->oi_primary->type != OSPF_IFTYPE_POINTOPOINT))?                  \
      &(I)->destination->u.prefix4 :                                          \
      &IPv4AllSPFRouters)

#define OSPF_PACKET_DST_BY_IF(I)                                             \
    (((I)->type == OSPF_IFTYPE_VIRTUALLINK                                   \
    || (CHECK_FLAG ((I)->flags, OSPF_MULTI_AREA_IF)                          \
      && (I)->oi_primary->type != OSPF_IFTYPE_POINTOPOINT))                  \
    ? &(I)->destination->u.prefix4 :                                         \
     (I)->state == IFSM_DROther ? &IPv4AllDRouters : &IPv4AllSPFRouters)

#define OSPF_PACKET_DST_BY_NBR(N)                                             \
    ((N)->oi->type == OSPF_IFTYPE_POINTOPOINT                                \
     && !CHECK_FLAG ((N)->oi->flags, OSPF_MULTI_AREA_IF) ?                    \
     &IPv4AllSPFRouters :       \
     ((N)->oi->type == OSPF_IFTYPE_VIRTUALLINK                                \
      || (CHECK_FLAG ((N)->oi->flags, OSPF_MULTI_AREA_IF)                     \
      && (N)->oi->oi_primary->type != OSPF_IFTYPE_POINTOPOINT))?              \
     &(N)->oi->destination->u.prefix4 : &(N)->ident.address->u.prefix4)

#else

#define OSPF_PACKET_DST_HELLO(I)                                              \
    ((I)->type == OSPF_IFTYPE_VIRTUALLINK ?                                   \
     &(I)->destination->u.prefix4 : &IPv4AllSPFRouters)

#define OSPF_PACKET_DST_BY_IF(I)                                              \
    ((I)->type == OSPF_IFTYPE_VIRTUALLINK ? &(I)->destination->u.prefix4 :    \
     (I)->state == IFSM_DROther ? &IPv4AllDRouters : &IPv4AllSPFRouters)

#define OSPF_PACKET_DST_BY_NBR(N)                                             \
    ((N)->oi->type == OSPF_IFTYPE_POINTOPOINT ? &IPv4AllSPFRouters :          \
     (N)->oi->type == OSPF_IFTYPE_VIRTUALLINK ?                               \
     &(N)->oi->destination->u.prefix4 : &(N)->ident.address->u.prefix4)

#endif /* HAVE_OSPF_MULTI_AREA */

#define IS_DD_FLAGS_SET(D,T)    (CHECK_FLAG ((D)->flags, OSPF_DD_ ## T))
#define IS_DD_FLAGS_SET_ALL(D)                                                \
    (((D)->flags & OSPF_DD_FLAG_ALL) == OSPF_DD_FLAG_ALL)

#define IS_DD_DUP(D,N)                                                        \
    ((D)->options == (N)->options &&                                          \
     (D)->flags == (N)->flags &&                                              \
     (D)->seqnum == (N)->seqnum)

#define LS_ACK_ADD(I,L)                                                       \
  do {                                                                        \
    vector_set ((I)->ls_ack, ospf_lsa_lock ((L)));                            \
    OSPF_IFSM_TIMER_ON ((I)->t_ls_ack, ospf_ls_ack_timer,                     \
                        OSPF_DELAYED_ACK_DELAY);                              \
  } while (0)

#define OSPF_PNT_UCHAR_PUT(P,O,V)                                             \
    do {                                                                      \
      *(((u_char *)(P)) + (O)) = (V);                                         \
    } while (0)

#define OSPF_PNT_UINT16_PUT(P,O,V)                                            \
    do {                                                                      \
      *(((u_char *)(P)) + (O)) = ((V) >> 8) & 0xFF;                           \
      *(((u_char *)(P)) + (O) + 1) = (V) & 0xFF;                              \
    } while (0)

#define OSPF_PNT_UINT24_PUT(P,O,V)                                            \
    do {                                                                      \
      *(((u_char *)(P)) + (O)) = ((V) >> 16) & 0xFF;                          \
      *(((u_char *)(P)) + (O) + 1) = ((V) >> 8) & 0xFF;                       \
      *(((u_char *)(P)) + (O) + 2) = (V) & 0xFF;                              \
    } while (0)

#define OSPF_PNT_UINT32_PUT(P,O,V)                                            \
    do {                                                                      \
      *(((u_char *)(P)) + (O)) = ((V) >> 24) & 0xFF;                          \
      *(((u_char *)(P)) + (O) + 1) = ((V) >> 16) & 0xFF;                      \
      *(((u_char *)(P)) + (O) + 2) = ((V) >> 8) & 0xFF;                       \
      *(((u_char *)(P)) + (O) + 3) = (V) & 0xFF;                              \
    } while (0)

/* Prototypes. */
void ospf_packet_add_unuse (struct ospf *, struct ospf_packet *);
void ospf_packet_unuse_clear (struct ospf *);
void ospf_packet_unuse_clear_half (struct ospf *);
void ospf_packet_unuse_max_update (struct ospf *, u_int32_t);

#ifdef HAVE_MD5
void ospf_packet_md5_digest_calc (struct ospf_crypt_key *,
                                  void *, u_int16_t, u_char *);
#endif /* HAVE_MD5 */

void ospf_packet_clear_all (struct ospf_interface *);
void ospf_packet_send_all (struct ospf_interface *);
u_int16_t ospf_packet_auth_type (struct ospf_header *);

void ospf_hello_send (struct ospf_interface *, struct pal_in4_addr *, int);
void ospf_hello_send_nbma (struct ospf_interface *, int);
int ospf_hello_timer (struct thread *);
int ospf_hello_poll_timer (struct thread *);
int ospf_hello_reply_event (struct thread *);
void ospf_db_desc_send (struct ospf_neighbor *);
void ospf_db_desc_resend (struct ospf_neighbor *, bool_t);
void ospf_ls_req_send (struct ospf_neighbor *);
void ospf_ls_req_event (struct ospf_neighbor *);
void ospf_ls_upd_send_lsa (struct ospf_interface *, struct ospf_lsa *);
void ospf_ls_upd_send_lsa_direct (struct ospf_neighbor *, struct ospf_lsa *);
void ospf_ls_upd_send_lsa_force (struct ospf_interface *,
                                 struct ospf_lsa *, struct pal_in4_addr *);
int ospf_ls_upd_send_event (struct thread *);
int ospf_ls_upd_timer_interval (struct ospf_neighbor *);
int ospf_ls_upd_timer (struct thread *);
int ospf_ls_ack_timer (struct thread *);
int ospf_ls_unack_count (struct thread *);

int ospf_read (struct thread *);

#endif /* _PACOS_OSPF_PACKET_H */
