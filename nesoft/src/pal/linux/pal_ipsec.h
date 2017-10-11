/* Copyright (C) 2005  All Rights Reserved. */
#ifndef PACOS_IPSEC_TMP_H
#define PACOS_IPSEC_TMP_H

#define MAX_RCVBUF_SIZE       (2048 - 1)

#ifdef HAVE_IPNET
/*
 *===========================================================================
 *                         SADB_EXT_ BITMAP
 *===========================================================================
 */
#define PF_KEY 15
#define PF_KEY_V2 2

#define IP_U64_COPY(dst,src)      ((dst) = (src))
#define IP_U64_EQUAL(x,y)         ((x) == (y))
#define IP_U64_NEQUAL(x,y)        ((x) != (y))
#define IP_U64_ISZERO(x)          ((x) == 0)
#define IP_U64_CLEAR(x)           ((x) = 0)
#define IP_U64_ADD(x,y)           ((x) += (y))
#define IP_U64_SUB(x,y)           ((x) -= (y))

#define SEBIT_SA                (1 << SADB_EXT_SA)
#define SEBIT_LIFETIME_CURRENT  (1 << SADB_EXT_LIFETIME_CURRENT)
#define SEBIT_LIFETIME_HARD     (1 << SADB_EXT_LIFETIME_HARD)
#define SEBIT_LIFETIME_SOFT     (1 << SADB_EXT_LIFETIME_SOFT)
#define SEBIT_ADDRESS_SRC       (1 << SADB_EXT_ADDRESS_SRC)
#define SEBIT_ADDRESS_DST       (1 << SADB_EXT_ADDRESS_DST)
#define SEBIT_ADDRESS_PROXY     (1 << SADB_EXT_ADDRESS_PROXY)
#define SEBIT_KEY_AUTH          (1 << SADB_EXT_KEY_AUTH)
#define SEBIT_KEY_ENCRYPT       (1 << SADB_EXT_KEY_ENCRYPT)
#define SEBIT_IDENTITY_SRC      (1 << SADB_EXT_IDENTITY_SRC)
#define SEBIT_IDENTITY_DST      (1 << SADB_EXT_IDENTITY_DST)
#define SEBIT_SENSITIVITY       (1 << SADB_EXT_SENSITIVITY)
#define SEBIT_PROPOSAL          (1 << SADB_EXT_PROPOSAL)
#define SEBIT_SUPPORTED         (1 << SADB_EXT_SUPPORTED)
#define SEBIT_SPIRANGE          (1 << SADB_EXT_SPIRANGE)
#define SEBIT_X_SRC_MASK        (1 << SADB_X_EXT_SRC_MASK)
#define SEBIT_X_DST_MASK        (1 << SADB_X_EXT_DST_MASK)
#define SEBIT_X_PROTOCOL        (1 << SADB_X_EXT_PROTOCOL)
#define SEBIT_X_SA2             (1 << SADB_X_EXT_SA2)
#define SEBIT_X_SRC_FLOW        (1 << SADB_X_EXT_SRC_FLOW)
#define SEBIT_X_DST_FLOW        (1 << SADB_X_EXT_DST_FLOW)
#define SEBIT_X_DST2            (1 << SADB_X_EXT_DST2)
#define SEBIT_X_FLOW_TYPE       (1 << SADB_X_EXT_FLOW_TYPE)
#define SEBIT_X_UDPENCAP        (1 << SADB_X_EXT_UDPENCAP)

/* Groups of bits. */
#define SEBIT_LIFETIME  (SEBIT_LIFETIME_CURRENT | SEBIT_LIFETIME_HARD | \
                         SEBIT_LIFETIME_SOFT)
#define SEBIT_ADDRESS   (SEBIT_ADDRESS_SRC | SEBIT_ADDRESS_DST | \
                         SEBIT_ADDRESS_PROXY)
#define SEBIT_KEY       (SEBIT_KEY_AUTH | SEBIT_KEY_ENCRYPT)
#define SEBIT_IDENTITY  (SEBIT_IDENTITY_SRC | SEBIT_IDENTITY_DST)
#define SEBIT_X_FLOW    (SEBIT_X_SRC_FLOW | SEBIT_X_DST_FLOW)
#define SEBIT_X_MASK    (SEBIT_X_SRC_MASK | SEBIT_X_DST_MASK)

#define IP_ROUNDUP(val,to)   ((val) % (to) ? ((val) + (to)) - ((val) % (to)) : (val))

#define EXT_SADBADDR_SIZE_MIN   (int)(sizeof (struct sadb_address) + IP_ROUNDUP(sizeof (struct Ip_sockaddr_in), 8))

#define EXT_SADBADDR_SIZE_IN    24 /* same expression as above. */ 
#define EXT_SADBADDR_SIZE_IN6   40 /* 8+sizeof(struct Ip_sockaddr_in6) = 8+28 = 36 -> rounded up = 40 */

typedef struct Extbits_struct
{
  u_int32_t  in_req;
  u_int32_t  in_opt;
} Extbits;

/*
 ****************************************************************************
 * 5  TYPES
 ****************************************************************************
 */


/*
 *===========================================================================
 *                      sadb_address_full
 *===========================================================================
 */
struct sadb_address_full
{
    struct sadb_address       addr;
    union Ip_sockaddr_union   sock;
    u_int8_t                     pad[8];
};


/*
 *===========================================================================
 *                         
 *===========================================================================
 */

/*
 *===========================================================================
 *                      Keyadm_sadb
 *===========================================================================
 */
typedef struct Keyadm_sadb_struct
{
    struct sadb_msg        *msg;
    struct sadb_sa         *sa;
    struct sadb_lifetime   *lifetime_current;
    struct sadb_lifetime   *lifetime_hard;
    struct sadb_lifetime   *lifetime_soft;
    struct sadb_address    *address_src;
    struct sadb_address    *address_dst;
    struct sadb_address    *address_proxy;
    struct sadb_key        *key_auth;
    struct sadb_key        *key_encrypt;
    struct sadb_ident      *ident_src;
    struct sadb_ident      *ident_dst;
    struct sadb_sens       *sens;
    struct sadb_prop       *prop;
    struct sadb_supported  *supported;
    struct sadb_spirange   *spirange;
    struct sadb_address    *src_mask;
    struct sadb_address    *dst_mask;
    struct sadb_protocol   *protocol;
    struct sadb_sa         *sa2;
    struct sadb_address    *src_flow;
    struct sadb_address    *dst_flow;
    struct sadb_address    *dst2;
    struct sadb_protocol   *flow_type;
}
Keyadm_sadb;

/*
 *===========================================================================
 *                         Variables - Vars
 *===========================================================================
 */
typedef struct Vars_struct
{
  int               argc;
  char              **argv;
  pal_sock_handle_t fd;
  void              *exthdr[SADB_EXT_MAX+1];
  Keyadm_sadb       *sadb;
  struct sadb_msg   sadbmsg;

  u_int8_t          *orgbuf;
  char              *stdoutbuf;
  u_int8_t          *msgbuf;

    /* options. */
  u_int32_t           minspi, maxspi;
  u_int32_t           spi_n, spi2_n;
  union Ip_sockaddr_union  srcaddr;
  union Ip_sockaddr_union  dstaddr;
  union Ip_sockaddr_union  proxyaddr;
  union Ip_sockaddr_union  dstaddr2;
  u_int8_t            satype, satype2;
  u_int8_t            have_satype;
  u_int8_t            flow_type;
  union Ip_sockaddr_union  srcmask;
  union Ip_sockaddr_union  dstmask;
  union Ip_sockaddr_union  srcflow;
  union Ip_sockaddr_union  dstflow;
  u_int16_t           dstport_n, srcport_n;
  u_int8_t            transport_proto;
  u_int8_t            replay;
  u_int32_t           ls_sec, lh_sec;

  s_int8_t            encrypt_type;
  u_int8_t            encrypt_len;
  struct sadb_key  encrypt_sadb;
  u_int8_t            encrypt_key[128];

  s_int8_t            auth_type;
  u_int8_t            auth_len;
  struct sadb_key  auth_sadb;
  u_int8_t            auth_key[32];

  u_int32_t           old;
  u_int32_t           halfiv;
  u_int32_t           forcetunnel;
  u_int32_t           delchain;
  u_int32_t           ingress, egress;
  u_int32_t           bypass;
  u_int32_t           replace;
  u_int32_t           test;
  u_int32_t           extmap;
  int                 silent;
}Vars;

int
pal_ipsec_delete_sa (struct ipsec_crypto_map_bundle *, struct interface *);

int
pal_ipsec_get_sas (struct nsm_sadb *, struct ipsec_crypto_map *,
                   struct interface *);

int
pal_ipsec_del_sa_flow (struct ipsec_crypto_map_bundle *,
                       struct interface *);
int
pal_ipsec_add_sa_flow (struct ipsec_crypto_map_bundle *,
                       struct interface *);
int
pal_ipsec_add_sa (struct ipsec_crypto_map_bundle *, struct interface *);
int
pal_ipsec_init ();
int
pal_ipsec_deinit ();
int
pal_ipsec_flush_all_sas (int proto_type);

int
pal_ipsec_ike_config (struct ipsec_crypto_map_bundle *);

/* IKE config file path. */
#define IPSEC_DEFAULT_CONF_ROOT           "/etc/interpeak/"
#define IPSEC_DEFAULT_FILENAME            IPSEC_DEFAULT_CONF_ROOT "ike.cfg"

extern char *sa_state[];

#define SA_STATE(x)   ((x > SADB_SASTATE_MAX) ? "ILLEGAL" : sa_state[x])

extern char *sa_auth[];

#define SA_AUTH(x)   ((x > SADB_AALG_MAX) ? "ILLEGAL" : sa_auth[x])

extern char *sa_encrypt[];
#define SA_ENCRYPT(x)   ((x > SADB_EALG_MAX) ? "ILLEGAL" : sa_encrypt[x])

#endif /* HAVE_IPNET */
#endif /* PACOS_IPSEC_TMP_H */
