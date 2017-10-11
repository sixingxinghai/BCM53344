/* Copyright (C) 2005  All Rights Reserved. */
#include "pal.h"
#include "pal_assert.h"
#include "pal_string.h"
#include "pal_ipsec_pfkeyv2.h"
#include "lib.h"
#include "if.h"
#include "thread.h"
#include "table.h"
#include "show.h"
#include "log.h"
#include "hash.h"
#include "prefix.h"
#include "snprintf.h"
#include "network.h"
#include "checksum.h"
#include "line.h"
#include "plist.h"
#include "log.h"
#include "ptree.h"
#include "stream.h"
#include "timeutil.h"
#include "nsm/ipsec/ipsec.h"
#include "nsm/nsmd.h"
#include "nsm/nsm_router.h"
#include "pal_ipsec.h"

#ifdef HAVE_IPNET


//static int sadbmsgseq = 1;
#define PKT_DUMP(A,B)                                      \
    do {                                                   \
      int i = 0, col = 0;                                  \
      static int count = 0;                                \
                                                           \
      printf("\nPacket %d, size %d:\n", ++count, (B));    \
      while (i < (B))                                      \
       {                                                   \
         printf("%02x ", A[i]);                           \
         i++;                                              \
         if (++col == 16)                                  \
          {                                                \
         printf("\n");                           \
            col = 0;                                       \
          }                                                \
       }                                                  \
      printf ("\n");                                       \
    } while (0)


char *sa_state[SADB_SASTATE_MAX+2] =
  {
    "LARVAL",
    "MATURE",
    "DYING",
    "DEAD",
    "\n"
  };

#define SA_STATE(x)   ((x > SADB_SASTATE_MAX) ? "ILLEGAL" : sa_state[x])

char *sa_auth[SADB_AALG_MAX+2] =
  {
    "NONE",
    "MD5HMAC",
    "SHA1HMAC",
    "MD5",
    "SHA1",
    "RMD160",
    "OLDMD5",
    "OLDSHA1",
    "AESMAC",
    "\n"
  };

#define SA_AUTH(x)   ((x > SADB_AALG_MAX) ? "ILLEGAL" : sa_auth[x])

char *sa_encrypt[SADB_EALG_MAX+2] =
  {
    "NONE",
    "DES",
    "3DES",
    "BLF",
    "CAST",
    "SKIPJACK",
    "AES",
    "\n"
  };

#define SA_ENCRYPT(x)   ((x > SADB_EALG_MAX) ? "ILLEGAL" : sa_encrypt[x])

int pal_keyadm_ext_sizes[SADB_EXT_MAX+1] =
  {   0,
      sizeof(struct sadb_sa),
      sizeof(struct sadb_lifetime), sizeof(struct sadb_lifetime), 
      sizeof(struct sadb_lifetime),
      -EXT_SADBADDR_SIZE_MIN, -EXT_SADBADDR_SIZE_MIN, -EXT_SADBADDR_SIZE_MIN,
#if 0
      -(int)sizeof(struct sadb_address), -(int)sizeof(struct sadb_address), 
      -(int)sizeof(struct sadb_address),
#endif
      -(int)sizeof(struct sadb_key), -(int)sizeof(struct sadb_key),
      -(int)sizeof(struct sadb_ident), -(int)sizeof(struct sadb_ident),
      -(int)sizeof(struct sadb_sens),
      -(int)sizeof(struct sadb_prop),
      -(int)sizeof(struct sadb_supported),
      sizeof(struct sadb_spirange),
      -EXT_SADBADDR_SIZE_MIN, -EXT_SADBADDR_SIZE_MIN, 
#if 0
      -(int)sizeof(struct sadb_address), -(int)sizeof(struct sadb_address), 
#endif
      sizeof(struct sadb_protocol),
      sizeof(struct sadb_sa),
      -EXT_SADBADDR_SIZE_MIN, -EXT_SADBADDR_SIZE_MIN,
      -EXT_SADBADDR_SIZE_MIN, 
#if 0
      -(int)sizeof(struct sadb_address), -(int)sizeof(struct sadb_address), 
      -(int)sizeof(struct sadb_address),
#endif 
      sizeof(struct sadb_protocol),
      sizeof(struct sadb_x_udpencap)
  };

  static Extbits extbits[SADB_MAX+1] =
    {
      /* FIELDS: (SADB_RESERVED) */
      { 0, /* in_req */
        0  /* in_opt */
      },
      /* SADB_GETSPI */
      { SEBIT_SPIRANGE | SEBIT_ADDRESS_SRC | SEBIT_ADDRESS_DST,
        SEBIT_SPIRANGE | SEBIT_ADDRESS_SRC | SEBIT_ADDRESS_DST
      },
      /* SADB_UPDATE */
      { SEBIT_SA | SEBIT_ADDRESS_SRC | SEBIT_ADDRESS_DST,
        SEBIT_SA | SEBIT_LIFETIME | SEBIT_ADDRESS | SEBIT_KEY | SEBIT_IDENTITY | \
        SEBIT_SENSITIVITY
      },
      /* SADB_ADD */
      { SEBIT_SA | SEBIT_ADDRESS_DST | SEBIT_ADDRESS_SRC,
        SEBIT_SA | SEBIT_LIFETIME | SEBIT_ADDRESS | SEBIT_KEY | SEBIT_IDENTITY | \
        SEBIT_SENSITIVITY
      },
      /* SADB_DELETE */
      { SEBIT_SA | SEBIT_ADDRESS_DST,
        SEBIT_SA | SEBIT_ADDRESS_SRC | SEBIT_ADDRESS_DST
      },
      /* SADB_GET */
      { SEBIT_SA | SEBIT_ADDRESS_SRC | SEBIT_ADDRESS_DST,
        SEBIT_SA | SEBIT_ADDRESS_SRC | SEBIT_ADDRESS_DST
      },
      /* SADB_ACQUIRE */
      { 0,
        SEBIT_ADDRESS | SEBIT_IDENTITY | SEBIT_SENSITIVITY | SEBIT_PROPOSAL
      },
      /* SADB_REGISTER */
      { 0,
        0
      },
      /* SADB_EXPIRE */
      { SEBIT_SA | SEBIT_ADDRESS_SRC | SEBIT_ADDRESS_DST,
        SEBIT_SA | SEBIT_ADDRESS_SRC | SEBIT_ADDRESS_DST
      },
      /* SADB_FLUSH */
      { 0,
        0
      },
      /* SADB_DUMP */
      { 0,
        0
      },
      /* SADB_X_PROMISC */
      { 0,
        0
      },
      /* SADB_X_ADDFLOW */
      { SEBIT_X_MASK | SEBIT_X_FLOW,
        SEBIT_SA | SEBIT_ADDRESS_DST | SEBIT_X_MASK | SEBIT_X_FLOW | \
        SEBIT_ADDRESS_SRC | SEBIT_X_PROTOCOL | SEBIT_X_FLOW_TYPE
      },
      /* SADB_X_DELFLOW */
      { SEBIT_X_MASK | SEBIT_X_FLOW,
        SEBIT_SA | SEBIT_X_MASK | SEBIT_X_FLOW | SEBIT_X_PROTOCOL | \
        SEBIT_ADDRESS_DST | SEBIT_X_FLOW_TYPE
      },
      /* SADB_X_GRPSPIS */
      { SEBIT_SA | SEBIT_X_SA2 | SEBIT_X_DST2 | SEBIT_ADDRESS_DST | \
        SEBIT_X_PROTOCOL,
        SEBIT_SA | SEBIT_X_SA2 | SEBIT_X_DST2 | SEBIT_ADDRESS_DST | \
        SEBIT_X_PROTOCOL
      },
      /* SADB_X_BINDSA */
      { SEBIT_SA | SEBIT_X_SA2 | SEBIT_X_DST2 | SEBIT_ADDRESS_DST | \
        SEBIT_X_PROTOCOL,
        SEBIT_SA | SEBIT_X_SA2 | SEBIT_X_DST2 | SEBIT_ADDRESS_DST | \
        SEBIT_X_PROTOCOL
      }
    };

static int
pal_keyadm_sacheck(Vars *vars)
{
  if(vars->test)
    return 0;

  if(vars->have_satype == 0)
    {
      zlog_info (nzg, "<keyadm> : missing modifier '-proto'. \n");
      return -1;
    }

  if(vars->satype == SADB_SATYPE_ESP)
    {
      if(vars->encrypt_type == -1)
        {
          zlog_info (nzg, "<keyadm> : missing modifier '-enc'. \n");
          return -1;
        }
      if((vars->encrypt_type > 0) && (vars->encrypt_len == 0))
        {
          zlog_info (nzg, "<keyadm> : missing modifier '-ekey'. \n");
          return -1;
        }
    }

  if((vars->satype == SADB_SATYPE_ESP) || (vars->satype == SADB_SATYPE_AH))
    {
      if(vars->auth_type == -1)
        {
          zlog_info (nzg, "<keyadm> : missing modifier '-auth'. \n");
          return -1;
        }
      if ((vars->auth_type > 0) && (vars->auth_len == 0))
        {
          zlog_info (nzg, "<keyadm> : missing modifier '-akey'. \n");
          return -1;
        }
    }

  return 0;
}

#define IPSEC_MAX (A, B)        ((A) > (B) ? (A) : (B))

/* 'val' is rounded up to 'to' */
#define IPSEC_ROUNDUP(val,to)   ((val) % (to) ? ((val) + (to)) - ((val) % (to)) : (val))

void
_pal_keyadm_ipaddr_to_extaddr (struct sadb_address_full *address, 
                              union Ip_sockaddr_union *sockaddr)
{
  int  addrsize = EXT_SADBADDR_SIZE_IN;

  if (sockaddr->sa.sa_family == AF_INET)
    addrsize = EXT_SADBADDR_SIZE_IN;
#ifdef HAVE_IPV6
  else if (sockaddr->sa.sa_family == AF_INET6)
    addrsize = EXT_SADBADDR_SIZE_IN6;
#endif /* HAVE_IPV6 */

  address->addr.sadb_address_len = (u_int16_t)(addrsize >> 3);
  address->addr.sadb_address_proto = 0; /*!! non-zero ports -> UDP OR TCP else 0. */
  address->addr.sadb_address_prefixlen = 0;
  address->addr.sadb_address_reserved = 0;

  pal_mem_cpy (&address->sock, sockaddr, sizeof(union Ip_sockaddr_union));
}

static int
_pal_keyadm_send_command (Vars *vars, u_int8_t type, u_int8_t satype)
{
  int retval = 0, i, length;
  u_int8_t *orgbuf, *msgbuf;
  struct sadb_address_full src, dst, dst2, proxy;
  struct sadb_address_full srcflow, dstflow, srcmask, dstmask;
  struct sadb_spirange  spi;
  struct sadb_sa sa, sa2;
  struct sadb_protocol proto2;
  struct sadb_lifetime ls, lh;
  struct sadb_protocol flow_type;
  struct nsm_master *nm;
  struct nsm_ipsec_master *ipsec_master = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (nzg);
  if (vr == NULL)
    {
      zlog_info (nzg, " Vr not found\n");
      return -1;
    }
  nm = nsm_master_lookup_by_id (nzg, vr->id);
  if (nm == NULL)
    {
      zlog_info (nzg, "NSM master not found \n");
      return -1;
    }

  ipsec_master = nm->ipsec_master;

  /** Auto build some extension headers. **/
  if(CHECK_FLAG(vars->extmap, SEBIT_SA))
    {
      /* SADB_EXT_SA */
      pal_mem_set(&sa, 0, sizeof(struct sadb_sa));
      vars->sadb->sa = &sa;
      vars->sadb->sa->sadb_sa_spi = vars->spi_n;
      vars->sadb->sa->sadb_sa_replay = vars->replay;
      vars->sadb->sa->sadb_sa_len = sizeof(struct sadb_sa) / 
        sizeof(ut_int64_t);
      if (vars->auth_type > 0)
        vars->sadb->sa->sadb_sa_auth = vars->auth_type;
      else
        vars->sadb->sa->sadb_sa_auth = 0;
      if (vars->encrypt_type > 0)
        vars->sadb->sa->sadb_sa_encrypt = vars->encrypt_type;
      else
        vars->sadb->sa->sadb_sa_encrypt = 0;
      vars->sadb->sa->sadb_sa_exttype = SADB_EXT_SA;
      if((type == SADB_UPDATE) || (type == SADB_ADD))
        {
          vars->sadb->sa->sadb_sa_state = SADB_SASTATE_MATURE;
          if(vars->old)
            {
              vars->sadb->sa->sadb_sa_flags |= SADB_X_SAFLAGS_NOREPLAY;
              if(vars->satype == SADB_SATYPE_ESP)
                {
                  vars->sadb->sa->sadb_sa_flags |= 
                    SADB_X_SAFLAGS_RANDOMPADDING;
                  if(vars->halfiv)
                    vars->sadb->sa->sadb_sa_flags |= SADB_X_SAFLAGS_HALFIV;
                }
            }
          if(vars->forcetunnel)
            vars->sadb->sa->sadb_sa_flags |= SADB_X_SAFLAGS_TUNNEL;
        }

      if((type == SADB_DELETE) && (vars->delchain))
        vars->sadb->sa->sadb_sa_flags |= SADB_X_SAFLAGS_CHAINDEL;

      if((type == SADB_X_ADDFLOW) || (type == SADB_X_DELFLOW))
        {
          if(vars->ingress)
            vars->sadb->sa->sadb_sa_flags |= SADB_X_SAFLAGS_INGRESS_FLOW;
          if(vars->replace)
            vars->sadb->sa->sadb_sa_flags |= SADB_X_SAFLAGS_REPLACEFLOW;
        }

      if((type == SADB_ADD) || (type == SADB_UPDATE) || 
         (type == SADB_GETSPI) || (type == SADB_GET) || 
         (type ==SADB_X_GRPSPIS))
        {
          if(vars->ingress)
            vars->sadb->sa->sadb_sa_flags |= SADB_X_SAFLAGS_INGRESS_SA;
          else if(vars->egress)
            vars->sadb->sa->sadb_sa_flags |= SADB_X_SAFLAGS_EGRESS_SA;
        }
    }

  /* SADB_EXT_SPIRANGE */
  if ((vars->maxspi) && CHECK_FLAG(vars->extmap, SEBIT_SPIRANGE))
    {
      vars->sadb->spirange = (struct sadb_spirange *)&spi;
      vars->sadb->spirange->sadb_spirange_len = 
        sizeof(struct sadb_spirange) / sizeof(ut_int64_t);
      vars->sadb->spirange->sadb_spirange_exttype = SADB_EXT_SPIRANGE;
      vars->sadb->spirange->sadb_spirange_min = vars->minspi;
      vars->sadb->spirange->sadb_spirange_max = vars->maxspi;
      vars->sadb->spirange->sadb_spirange_reserved = 0;
    }
  /* SADB_EXT_ADDRESS_SRC */
  if(CHECK_FLAG(vars->extmap, SEBIT_ADDRESS_SRC))
    {
      vars->sadb->address_src = (struct sadb_address *)&src;
      _pal_keyadm_ipaddr_to_extaddr(&src, &vars->srcaddr);
      vars->sadb->address_src->sadb_address_exttype = SADB_EXT_ADDRESS_SRC;
    }

  /* SADB_EXT_ADDRESS_DST */
  if(CHECK_FLAG(vars->extmap, SEBIT_ADDRESS_DST))
    {
      vars->sadb->address_dst = (struct sadb_address *)&dst;
      _pal_keyadm_ipaddr_to_extaddr(&dst, &vars->dstaddr);
      vars->sadb->address_dst->sadb_address_exttype = SADB_EXT_ADDRESS_DST;
    }

  /* SADB_EXT_ADDRESS_PROXY */
  if(CHECK_FLAG(vars->extmap, SEBIT_ADDRESS_PROXY))
    {
      vars->sadb->address_proxy = (struct sadb_address *)&proxy;
      _pal_keyadm_ipaddr_to_extaddr(&proxy, &vars->proxyaddr);
      vars->sadb->address_proxy->sadb_address_exttype = 
        SADB_EXT_ADDRESS_PROXY;
    }

  /* SADB_EXT_KEY_ENCRYPT */
  if (CHECK_FLAG(extbits[type].in_opt, SEBIT_KEY) && (vars->encrypt_len > 0))
    {
      vars->sadb->key_encrypt = &vars->encrypt_sadb;
      vars->sadb->key_encrypt->sadb_key_len = 
        (uint16_t)((sizeof(struct sadb_key) + 
                    IPSEC_ROUNDUP(vars->encrypt_len, 8)) / sizeof(ut_int64_t));
      vars->sadb->key_encrypt->sadb_key_exttype = SADB_EXT_KEY_ENCRYPT;
      vars->sadb->key_encrypt->sadb_key_bits = 
        (uint16_t)(vars->encrypt_len * 8);
      vars->sadb->key_encrypt->sadb_key_reserved = 0;
    }

  /* SADB_EXT_KEY_AUTH */
  if(CHECK_FLAG(extbits[type].in_opt, SEBIT_KEY) && (vars->auth_len > 0))
    {
      vars->sadb->key_auth = &vars->auth_sadb;
      vars->sadb->key_auth->sadb_key_len = 
        (uint16_t)((sizeof(struct sadb_key) + 
                    IPSEC_ROUNDUP(vars->auth_len, 8)) / sizeof(ut_int64_t));
      vars->sadb->key_auth->sadb_key_exttype = SADB_EXT_KEY_AUTH;
      vars->sadb->key_auth->sadb_key_bits = (uint16_t)(vars->auth_len * 8);
      vars->sadb->key_auth->sadb_key_reserved = 0;
    }

  /* SEBIT_X_SRC_FLOW */
  if(CHECK_FLAG(vars->extmap, SEBIT_X_SRC_FLOW))
    {
      vars->sadb->src_flow = (struct sadb_address *)&srcflow;
      _pal_keyadm_ipaddr_to_extaddr(&srcflow, &vars->srcflow);
      srcflow.sock.sin.sin_port = vars->srcport_n;
      vars->sadb->src_flow->sadb_address_exttype = SADB_X_EXT_SRC_FLOW;
    }

  /* SEBIT_X_DST_FLOW */
  if(CHECK_FLAG(vars->extmap, SEBIT_X_DST_FLOW))
    {
      vars->sadb->dst_flow = (struct sadb_address *)&dstflow;
      _pal_keyadm_ipaddr_to_extaddr(&dstflow, &vars->dstflow);
      dstflow.sock.sin.sin_port = vars->dstport_n;
      vars->sadb->dst_flow->sadb_address_exttype = SADB_X_EXT_DST_FLOW;
    }

  /* SEBIT_X_SRC_MASK */
  if(CHECK_FLAG(vars->extmap, SEBIT_X_SRC_MASK))
    {
      vars->sadb->src_mask = (struct sadb_address *)&srcmask;
      _pal_keyadm_ipaddr_to_extaddr(&srcmask, &vars->srcmask);
      vars->sadb->src_mask->sadb_address_exttype = SADB_X_EXT_SRC_MASK;
    }
  /* SEBIT_X_DST_MASK */
  if(CHECK_FLAG(vars->extmap, SEBIT_X_DST_MASK))
    {
      vars->sadb->dst_mask = (struct sadb_address *)&dstmask;
      _pal_keyadm_ipaddr_to_extaddr(&dstmask, &vars->dstmask);
      vars->sadb->dst_mask->sadb_address_exttype = SADB_X_EXT_DST_MASK;
    }

  /* SEBIT_X_DST2 */
  if(CHECK_FLAG(vars->extmap, SEBIT_X_DST2))
    {
      vars->sadb->dst2 = (struct sadb_address *)&dst2;
      _pal_keyadm_ipaddr_to_extaddr(&dst2, &vars->dstaddr2);
      vars->sadb->dst2->sadb_address_exttype = SADB_X_EXT_DST2;
    }

  /* SEBIT_X_SA2 */
  if(CHECK_FLAG(vars->extmap, SEBIT_X_SA2))
    {
      pal_mem_set (&sa2, 0, sizeof(struct sadb_sa));
      vars->sadb->sa2 = &sa2;
      vars->sadb->sa2->sadb_sa_spi = vars->spi2_n;
      vars->sadb->sa2->sadb_sa_len = 
        sizeof(struct sadb_sa) / sizeof(ut_int64_t);
      vars->sadb->sa2->sadb_sa_exttype = SADB_X_EXT_SA2;
      vars->sadb->sa2->sadb_sa_state = SADB_SASTATE_MATURE;
    }

  /* SEBIT_LIFETIME_SOFT - soft lifetime */
  if(CHECK_FLAG(vars->extmap, SEBIT_LIFETIME_SOFT))
    {
      pal_mem_set (&ls, 0, sizeof(struct sadb_lifetime));
      vars->sadb->lifetime_soft = &ls;
      vars->sadb->lifetime_soft->sadb_lifetime_len = 
        sizeof(struct sadb_lifetime) / sizeof(ut_int64_t);
      vars->sadb->lifetime_soft->sadb_lifetime_exttype = 
        SADB_EXT_LIFETIME_SOFT;
      IP_U64_SETLO(vars->sadb->lifetime_soft->sadb_lifetime_addtime, 
                   vars->ls_sec);
    }

  /* SEBIT_LIFETIME_HARD - hard lifetime */
  if(CHECK_FLAG(vars->extmap, SEBIT_LIFETIME_HARD))
    {
      pal_mem_set (&lh, 0, sizeof(struct sadb_lifetime));
      vars->sadb->lifetime_hard = &lh;
      vars->sadb->lifetime_hard->sadb_lifetime_len = 
        sizeof(struct sadb_lifetime) / sizeof(ut_int64_t);
      vars->sadb->lifetime_hard->sadb_lifetime_exttype = 
        SADB_EXT_LIFETIME_HARD;
      IP_U64_SETLO(vars->sadb->lifetime_hard->sadb_lifetime_addtime, 
                   vars->lh_sec);
    }

  /* SEBIT_X_PROTOCOL - security protocol */
  if(CHECK_FLAG(vars->extmap, SEBIT_X_PROTOCOL))
    {
      pal_mem_set (&proto2, 0, sizeof(struct sadb_protocol));
      vars->sadb->protocol = (struct sadb_protocol *)&proto2;
      vars->sadb->protocol->sadb_protocol_len = 
        sizeof(struct sadb_protocol) / sizeof(ut_int64_t);
      vars->sadb->protocol->sadb_protocol_exttype = SADB_X_EXT_PROTOCOL;
      vars->sadb->protocol->sadb_protocol_proto = vars->satype2;
    }
  /* SEBIT_X_PROTOCOL - transport protocol */
  else if(vars->transport_proto)
    {
      pal_mem_set (&proto2, 0, sizeof(struct sadb_protocol));
      vars->sadb->protocol = (struct sadb_protocol *)&proto2;
      vars->sadb->protocol->sadb_protocol_len = 
        sizeof(struct sadb_protocol) / sizeof(ut_int64_t);
      vars->sadb->protocol->sadb_protocol_exttype = SADB_X_EXT_PROTOCOL;
      vars->sadb->protocol->sadb_protocol_proto = vars->transport_proto;
    }

  /* SEBIT_X_FLOW_TYPE - flow type */
  if(type == SADB_X_ADDFLOW && CHECK_FLAG(vars->extmap, SEBIT_X_FLOW_TYPE))
    {
      pal_mem_set (&flow_type, 0, sizeof(struct sadb_protocol));
      vars->sadb->flow_type = (struct sadb_protocol *)&flow_type;
      vars->sadb->flow_type->sadb_protocol_len = 
        sizeof(struct sadb_protocol) / sizeof(ut_int64_t);
      vars->sadb->flow_type->sadb_protocol_exttype = SADB_X_EXT_FLOW_TYPE;
      vars->sadb->flow_type->sadb_protocol_proto = vars->flow_type;
    }
  for (length = sizeof(struct sadb_msg), i = 1; i <= SADB_EXT_MAX; i++)
    if (vars->exthdr[i] != NULL)
      length += (((struct sadb_ext *)vars->exthdr[i])->sadb_ext_len * 
                 sizeof(ut_int64_t));

  /* Allocate memory for the PFKEY write. */
  /* Replace ipcom_malloc with XCALLOC */
  orgbuf = msgbuf = (u_int8_t *) XCALLOC (MTYPE_IPSEC, length + 
                                          sizeof(ut_int64_t));
  if(msgbuf == NULL)
    {
      return -1;
    }
  if((u_int32_t)msgbuf & 0x7L)
    msgbuf += (sizeof(ut_int64_t) - ((u_int32_t)orgbuf & 0x7L)); 
  /* 64-bit align msgbuf. */
  /* Replace ip_assert with pal_assert */
  pal_assert(((u_int32_t)msgbuf & 0x7L) == 0);
  pal_mem_set(msgbuf, 0, length+sizeof(ut_int64_t));

  /* Set SADB_MSG */
  vars->sadb->msg->sadb_msg_version = PF_KEY_V2;
  vars->sadb->msg->sadb_msg_type = type;
  vars->sadb->msg->sadb_msg_errno = 0;
  vars->sadb->msg->sadb_msg_satype = satype;
  vars->sadb->msg->sadb_msg_reserved = 0;
  vars->sadb->msg->sadb_msg_len = (uint16_t)(length / sizeof(ut_int64_t));

  /* VIV */
  vars->sadb->msg->sadb_msg_seq = 1;
  vars->sadb->msg->sadb_msg_pid = 0xf;
  /* VIV */

  /* Copy over all headers. */
  pal_mem_cpy(msgbuf, vars->sadb->msg, sizeof(struct sadb_msg));
  for (length = sizeof(struct sadb_msg), i = 1; i <= SADB_EXT_MAX; i++)
    if (vars->exthdr[i] != NULL)
      {
        char *viv;
        int len;

        viv = vars->exthdr[i];
        len = ((struct sadb_ext *) vars->exthdr[i])->sadb_ext_len * sizeof(ut_int64_t);
        //PKT_DUMP (viv, len);
        pal_mem_cpy(msgbuf + length, vars->exthdr[i], ((struct sadb_ext *)
                                                       vars->exthdr[i])->sadb_ext_len * sizeof(ut_int64_t));
        length += ((struct sadb_ext *)vars->exthdr[i])->sadb_ext_len * 
          sizeof(ut_int64_t);
      }

  //PKT_DUMP (msgbuf, length);

  /* Write the SADB message. */ /*Replace ipcom_send with pal_sock_send */
  int val;
  val = pal_sock_send(ipsec_master->ipsec_fd, (char*)msgbuf, length, 0);
  zlog_info (nzg, "Return value is %d", val);
  if (val != length)
    {
      zlog_info (nzg, "nsm_keyadm_send_command: pal_sock_send failed \n");
      retval = -1;
    }

  if (orgbuf)
    XFREE (MTYPE_IPSEC, orgbuf);

  return retval;
}

static int
pal_keyadm_print_command(Vars *vars)
{
  union Ip_sockaddr_union   *sockaddr;
#ifdef HAVE_IPV6
  char str[46];
#else
  char str[16];
#endif
  int   addr_offset;
  time_t  addtime, usetime;
  u_int32_t bytes;

  if(vars->sadb->sa)
    {
      zlog_info (nzg, "      SA: spi=%x  sta=%s  auth=%s  enc=%s  replay=%ld \
                 fl=0x%x",
                 vars->sadb->sa->sadb_sa_spi,
                 SA_STATE(vars->sadb->sa->sadb_sa_state),
                 SA_AUTH(vars->sadb->sa->sadb_sa_auth),
                 SA_ENCRYPT(vars->sadb->sa->sadb_sa_encrypt),
                 (int)vars->sadb->sa->sadb_sa_replay,
                 vars->sadb->sa->sadb_sa_flags);
    }
  if(vars->sadb->address_src)
    {
      sockaddr = (union Ip_sockaddr_union *) \
        ((u_int8_t *)vars->sadb->address_src + sizeof(struct sadb_address));
      addr_offset = (sockaddr->sa.sa_family == AF_INET) ? 2 : 6;

      zlog_info (nzg, "     SRC: addr=%s",
                 pal_inet_ntop(sockaddr->sa.sa_family,
                 &sockaddr->sa.sa_data[addr_offset],
                 str, sizeof(str)));
    }

  if(vars->sadb->address_dst)
    {
      sockaddr = (union Ip_sockaddr_union *)((u_int8_t *) \
                                             vars->sadb->address_dst + sizeof(struct sadb_address));
      addr_offset = (sockaddr->sa.sa_family == AF_INET) ? 2 : 6;

      zlog_info (nzg, "     DST: addr=%s",
                 pal_inet_ntop (sockaddr->sa.sa_family, 
                                &sockaddr->sa.sa_data[addr_offset],
                                str, sizeof(str)));
    }
  
  if(vars->sadb->address_proxy)
    {
      sockaddr = (union Ip_sockaddr_union *)((u_int8_t *) \
                                             vars->sadb->address_proxy + sizeof(struct sadb_address));
      addr_offset = (sockaddr->sa.sa_family == AF_INET) ? 2 : 6;
      zlog_info (nzg, "   PROXY: addr=%s",
                 pal_inet_ntop(sockaddr->sa.sa_family,
                 &sockaddr->sa.sa_data[addr_offset],
                 str, sizeof(str)));
    }

  if(vars->sadb->spirange)
    {
      zlog_info (nzg, "  SPIRANGE: min=%d  max=%d",
                 vars->sadb->spirange->sadb_spirange_min,
                 vars->sadb->spirange->sadb_spirange_max);
    }

  /* IP_U64_GETLO was used */
  if(vars->sadb->lifetime_current)
    {
      addtime = (time_t) IP_U64_GETLO \
        (vars->sadb->lifetime_current->sadb_lifetime_addtime);
      usetime = (time_t) IP_U64_GETLO \
        (vars->sadb->lifetime_current->sadb_lifetime_usetime);
      bytes   = IP_U64_GETLO \
        (vars->sadb->lifetime_current->sadb_lifetime_bytes);
      zlog_info (nzg, " CURRENT: addtime=%ld usetime=%ld bytes=%ld",
                 addtime, usetime, bytes);
    }

  if(vars->sadb->lifetime_soft)
    {
      addtime = (time_t) IP_U64_GETLO \
        (vars->sadb->lifetime_soft->sadb_lifetime_addtime);
      usetime = (time_t) IP_U64_GETLO \
        (vars->sadb->lifetime_soft->sadb_lifetime_usetime);
      bytes   = IP_U64_GETLO \
        (vars->sadb->lifetime_soft->sadb_lifetime_bytes);
      zlog_info (nzg, "  SPIRANGE: min=%d  max=%d",
                 vars->sadb->spirange->sadb_spirange_min,
                 vars->sadb->spirange->sadb_spirange_max);
    }
  if(vars->sadb->lifetime_soft)
    {
      addtime = (time_t) IP_U64_GETLO \
        (vars->sadb->lifetime_soft->sadb_lifetime_addtime);
      usetime = (time_t) IP_U64_GETLO \
        (vars->sadb->lifetime_soft->sadb_lifetime_usetime);
      bytes   = IP_U64_GETLO \
        (vars->sadb->lifetime_soft->sadb_lifetime_bytes);
      zlog_info (nzg, "    SOFT: addtime=%ld usetime=%ld bytes=%ld",
                 addtime, usetime, bytes);
    }
  if(vars->sadb->lifetime_hard)
    {
      addtime = (time_t) IP_U64_GETLO \
        (vars->sadb->lifetime_hard->sadb_lifetime_addtime);
      usetime = (time_t) IP_U64_GETLO \
        (vars->sadb->lifetime_hard->sadb_lifetime_usetime);
      bytes   = IP_U64_GETLO \
        (vars->sadb->lifetime_hard->sadb_lifetime_bytes);
      zlog_info (nzg, "    HARD: addtime=%ld usetime=%ld bytes=%ld",
                 addtime, usetime, bytes);
    }


  return vars->sadb->msg->sadb_msg_errno == 0 ? 0 : -1;
}

static int
pal_keyadm_receive_command (Vars *vars)
{
  int length;
  int retval = -1;
  /*u_int32_t   extmap = 0;*/
  struct sadb_ext   *ext;
  struct nsm_master *nm;
  struct nsm_ipsec_master *ipsec_master = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (nzg);
  if (vr == NULL)
    {
      zlog_info (nzg, " Vr not found\n");
      return -1;
    }
  nm = nsm_master_lookup_by_id (nzg, vr->id);
  if (nm == NULL)
    {
      zlog_info (nzg, "NSM master not found \n");
      return -1;
    }

  ipsec_master = nm->ipsec_master;

  /* Release previous memory. */
  if(vars->orgbuf)
    {
      XFREE (MTYPE_IPSEC, vars->orgbuf);
      vars->orgbuf = NULL;
    }

  /* Prepare ext array. */
  pal_mem_set(&vars->sadb->sa, 0, SADB_EXT_MAX * sizeof(void*)); 
  /*lint !e419 */

  /* Get buffer memory. */
  vars->msgbuf = vars->orgbuf = (u_int8_t *) XCALLOC (MTYPE_IPSEC, 
                                                      MAX_RCVBUF_SIZE + sizeof(ut_int64_t));
  if (vars->msgbuf == NULL)
    {
      zlog_info (nzg, " ERR : out of memory, can't receive.\n");
      goto leave;
    }
  if ((u_int32_t)vars->msgbuf & 0x7L)
    vars->msgbuf += (sizeof(ut_int64_t) - ((u_int32_t)vars->orgbuf & 0x7L)); 
  /* 64-bit align msgbuf. */
  pal_assert(((u_int32_t)vars->msgbuf & 0x7L) == 0);
  pal_mem_set(vars->msgbuf, 0, MAX_RCVBUF_SIZE);

  /* Read a SADB message. */
  vars->sadb->msg = (struct sadb_msg *)vars->msgbuf;
  length = (int)pal_sock_recv(ipsec_master->ipsec_fd, (char *)vars->msgbuf, 
                              MAX_RCVBUF_SIZE, 0);
  if (length < (int)sizeof(struct sadb_msg))
    {
      if (length < 0)
        zlog_info(nzg, " ERR : read() length < 0 \n");
      else if (length >= 3 && vars->sadb->msg->sadb_msg_errno != 0)
        zlog_info(nzg, " ERR : read() msgerrno=%d \n", 
                  vars->sadb->msg->sadb_msg_errno);
      else
        zlog_info(nzg, " ERR : read() truncated SADB (%d). \n", length);
      goto leave;
    }
  else if(length == MAX_RCVBUF_SIZE)
    zlog_info(nzg, " ERR : received too large SADB (%d). \n", length);

  /* Verify the SADB_MSG header. */
  if(vars->sadb->msg->sadb_msg_version != PF_KEY_V2)
    zlog_info(nzg, " ERR : illegal sadb_msg_vesion (%d). \n", 
              vars->sadb->msg->sadb_msg_version);
  else if(vars->sadb->msg->sadb_msg_type > SADB_MAX ||
          vars->sadb->msg->sadb_msg_type == SADB_RESERVED)
    zlog_info(nzg, " ERR : illegal sadb_msg_type (%d). \n", 
              vars->sadb->msg->sadb_msg_type);
  else if(vars->sadb->msg->sadb_msg_reserved != 0)
    zlog_info(nzg, " ERR : illegal sadb_msg_reserved (%d). \n", 
              vars->sadb->msg->sadb_msg_reserved);
  else if ((int) (vars->sadb->msg->sadb_msg_len * 
                  sizeof(ut_int64_t)) != length)
    zlog_info(nzg, " ERR : length mismatch (sadb_msg_len=%d, length=%d). \n",
              vars->sadb->msg->sadb_msg_len * sizeof(ut_int64_t), length);

  vars->msgbuf += sizeof(struct sadb_msg);
  length -= sizeof(struct sadb_msg);
  /* Verify the extensions. */
  for(ext = (struct sadb_ext *)vars->msgbuf;
      length >= sizeof(struct sadb_ext);
      ext = (struct sadb_ext *)vars->msgbuf)
    {
      int   ext_len;

      /* extension headers must start at 64-bit alignment. */
      if (((u_int32_t)ext & 0x7L) != 0)
        zlog_info (nzg, " ERR : bad alignment for extension %d \n", 
                   ext->sadb_ext_type);

      ext_len = ext->sadb_ext_len * sizeof(ut_int64_t);
      if ((ext_len == 0) || (length < ext_len))
        zlog_info (nzg, " ERR : illegal EXT length (%d). \n", ext_len);
      if(ext->sadb_ext_type == SADB_EXT_RESERVED)
        zlog_info(nzg, " ERR : illegal EXT type (%d). \n", 
                  ext->sadb_ext_type);
      if (ext->sadb_ext_type > SADB_EXT_MAX)
        zlog_info (nzg, " ERR : unknown EXT type (%d). \n", 
                   ext->sadb_ext_type);
      else
        {
          /*extmap |= (1 << ext->sadb_ext_type);*/

          /* [2.3] Only one instance of an extension header. */
          if (vars->exthdr[ext->sadb_ext_type] != NULL) /* It was IP_NULL */
            zlog_info (nzg, " ERR : duplicate EXT type (%d). \n", 
                       ext->sadb_ext_type);
          else
            vars->exthdr[ext->sadb_ext_type] = ext;

          /* _Exact_ extension header size check. */
          if((pal_keyadm_ext_sizes[ext->sadb_ext_type] > 0) &&
             (pal_keyadm_ext_sizes[ext->sadb_ext_type] != ext_len))
            return EINVAL;

          /* _Minimum_ extension header size check. */
          if ((pal_keyadm_ext_sizes[ext->sadb_ext_type] < 0) &&
              (ext_len < pal_keyadm_ext_sizes[ext->sadb_ext_type]))
            return EINVAL;

          /* next extension header. */
          length -= ext_len;
          vars->msgbuf += ext_len;
        }
    }

  if(length != 0)
    zlog_info(nzg, " ERR : trailing SADB bytes (%d). \n", length);
  return 0;

 leave:
  return retval;
}

static int
_pal_keyadm_flush(Vars *vars)
{
  if(_pal_keyadm_send_command(vars, SADB_FLUSH, vars->satype) == 0)
    {
      (void)pal_keyadm_receive_command(vars);
      return pal_keyadm_print_command(vars);
    }

  return -1;
}

/* API to read the msg from the kernel */
static int
_ipsec_read (struct thread *ipsec_thread)
{
  struct nsm_master *nm;
  u_char msgbuf[MAX_RCVBUF_SIZE];
  struct nsm_ipsec_master *ipsec_master = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (nzg);
  int length;

  nm = nsm_master_lookup_by_id (nzg, vr->id);
  if (nm == NULL)
    {
      zlog_info (nzg, "NSM master not found \n");
      return -1;
    }

  ipsec_master = nm->ipsec_master;

  /* Add next event. */
  ipsec_master->ipsec_thread = thread_add_read (nzg, _ipsec_read, NULL, 
                                                ipsec_master->ipsec_fd);
   

  length = (int)pal_sock_recv(ipsec_master->ipsec_fd, (char *)msgbuf, 
                              MAX_RCVBUF_SIZE, 0);
 
  return length; 
}


/* Initialize the socket and schedule the read thread */
int
pal_ipsec_init ()
{
  struct nsm_master *nm;
  struct nsm_ipsec_master *ipsec_master = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (nzg);
  int ipsec_fd;
  struct thread *ipsec_thread = NULL;

  nm = nsm_master_lookup_by_id (nzg, vr->id);
  if (nm == NULL)
    {
      zlog_info (nzg, "NSM master not found \n");
      return -1;
    }

  ipsec_master = nm->ipsec_master;

  ipsec_fd = pal_sock (nzg, PF_KEY, SOCK_RAW, PF_KEY_V2);

  /* check for invalid socket */
  if (ipsec_fd == -1)
    {
      zlog_info (nzg, "ipsec_fill_vars: socket() failed \n");
      return -1;
    }

  /* launch thread to read back messages from the kernel. */
  ipsec_thread = thread_add_read (nzg, _ipsec_read, NULL, ipsec_fd);
  if (ipsec_thread == NULL)
    return -1;

  ipsec_master->ipsec_fd = ipsec_fd;
  ipsec_master->ipsec_thread = ipsec_thread;

  return 0;

}

/* Close the socket and cancel the read thread */
int
pal_ipsec_deinit ()
{
  struct nsm_master *nm;
  struct nsm_ipsec_master *ipsec_master = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (nzg);

  nm = nsm_master_lookup_by_id (nzg, vr->id);
  if (nm == NULL)
    {
      zlog_info (nzg, "NSM master not found \n");
      return -1;
    }

  ipsec_master = nm->ipsec_master;

  /* Close the fd */
  pal_sock_close (nzg, ipsec_master->ipsec_fd);
  /* Cancel the thread */
  thread_cancel (ipsec_master->ipsec_thread);
  /* Make the thread NULL */
  ipsec_master->ipsec_thread = NULL;

  return 0;
}

/* API to flush all the SAs in the kernel */
int
pal_ipsec_flush_all_sas (int proto_type)
{
  Vars vars;

  /* prepare data */
  pal_mem_set (&vars, 0, sizeof(Vars));

  vars.sadb = (Keyadm_sadb *)vars.exthdr;
  vars.sadb->msg = &vars.sadbmsg;
  vars.encrypt_type = -1;
  vars.auth_type = -1;
  if ((proto_type >= SADB_SATYPE_UNSPEC) && 
      (proto_type <= SADB_X_SATYPE_BYPASS))
    vars.satype = proto_type;

  /* Flush all SAs. */
  _pal_keyadm_flush (&vars); 
 
  return 0; 
}

static int
_ipsec_fill_vars (struct ipsec_crypto_map *crypto_map, 
                  struct interface *ifp, int flag,
                  Vars *vars, afi_t afi)
{
  struct ipsec_transform_set *transform_set;
  struct connected *conn_addr = NULL;
  struct ipsec_peer_addr *peer_addr = NULL;
  struct ipsec_ah_session_key *ah_session_key = NULL;
  struct ipsec_esp_session_key *esp_session_key = NULL;
  struct listnode *node = NULL;

  if (! crypto_map || ! crypto_map->transform_set)
    return -1;

  transform_set = listnode_head (crypto_map->transform_set);
  
  if (transform_set == NULL)
    return -1;
 
  /* prepare data */
  vars->sadb = (Keyadm_sadb *)vars->exthdr;
  vars->sadb->msg = &vars->sadbmsg;
  vars->encrypt_type = -1;
  vars->auth_type = -1;

  /* For not to print */
  vars->silent = 1;

  /* spi*/
  vars->spi_n = pal_hton32 (crypto_map->spi);

  /* Force tunnel at this point. */
  vars->forcetunnel = 1;

  /* Have SA type. */
  vars->have_satype = 1;
  vars->extmap = SEBIT_SA;

  /* Destination address. */
  /* Get the first peer configured from the peer_list and add it */
  LIST_LOOP( crypto_map->peer_addr, peer_addr, node)
    {
      if (afi == AFI_IP)
        {
          if (peer_addr->peer_addr.addr.family != AF_INET)
            continue;

          vars->dstaddr.sin.sin_len = sizeof (struct pal_sockaddr_in4);
          vars->dstaddr.sin.sin_family = AF_INET;
          vars->dstaddr.sin.sin_addr.s_addr = peer_addr->peer_addr.addr.prefix.s_addr;
        }
#ifdef HAVE_IPV6
      else if (afi == AFI_IP6)
        {
          if (peer_addr->peer_addr.ipv6_addr.family != AF_INET6)
            continue;

          vars->dstaddr.sin6.sin6_len = sizeof (struct pal_sockaddr_in6);
          vars->dstaddr.sin6.sin6_family = AF_INET6;
          pal_mem_cpy (&vars->dstaddr.sin6.sin6_addr, &peer_addr->peer_addr.ipv6_addr.prefix, sizeof (struct pal_in6_addr));
        }
#endif /* HAVE_IPV6 */

      vars->extmap |= SEBIT_ADDRESS_DST;
      break;
    }

  /* Source address. */
  if (afi == AFI_IP)
    {
      for (conn_addr = ifp->ifc_ipv4; conn_addr; conn_addr = conn_addr->next)
        if (conn_addr->address 
            && ! CHECK_FLAG (conn_addr->flags, NSM_IFA_SECONDARY))
          {
            vars->srcaddr.sin.sin_len = sizeof (struct pal_sockaddr_in4);
            vars->srcaddr.sin.sin_family = AF_INET;
            vars->srcaddr.sin.sin_addr.s_addr = conn_addr->address->u.prefix4.s_addr;
            vars->extmap |= SEBIT_ADDRESS_SRC;
          }
    }
#ifdef HAVE_IPV6
  else
    {
      /* Global addresses have to be configured for now. TBD. */
      for (conn_addr = ifp->ifc_ipv6; conn_addr; conn_addr = conn_addr->next)
        if (conn_addr->address
            && ! IN6_IS_ADDR_LINKLOCAL (&conn_addr->address->u.prefix6))
          {
            vars->srcaddr.sin6.sin6_len = sizeof (struct pal_sockaddr_in6);
            vars->srcaddr.sin6.sin6_family = AF_INET6;
            pal_mem_cpy (&vars->srcaddr.sin6.sin6_addr, &conn_addr->address->u.prefix6, sizeof (struct pal_in6_addr));
            vars->extmap |= SEBIT_ADDRESS_SRC;
          }
    }
#endif /* HAVE_IPV6 */

  if (transform_set->protocol == PROTO_AH)
    vars->satype = SADB_SATYPE_AH;
  else if (transform_set->protocol == PROTO_ESP)
    vars->satype =  SADB_SATYPE_ESP;

  //if (crypto_map->isakmp)
  //  return 0;

  switch (vars->satype)
    {
    case SADB_SATYPE_ESP:
      /* encrypt_key */
      /* Get the first esp session key and encode the key */
      if (! crypto_map->esp_session_key)
        return -1;

      esp_session_key = listnode_head (crypto_map->esp_session_key);

      if (transform_set->esp_enc_transform != TRANSFORM_ESP_ENC_NONE)
        pal_mem_cpy (vars->encrypt_key, esp_session_key->esp_enc_key, 
                     sizeof (vars->encrypt_key));

      /* encrypt_type */
      vars->encrypt_type = transform_set->esp_enc_transform;
      vars->encrypt_len = esp_session_key->esp_enc_key_len;

      if (transform_set->esp_auth_transform != TRANSFORM_ESP_AUTH_NONE)
        pal_mem_cpy (vars->auth_key, esp_session_key->esp_auth_key, 
                     sizeof (vars->auth_key));

      /* auth */
      vars->auth_type = transform_set->esp_auth_transform;
      vars->auth_len = esp_session_key->esp_auth_key_len;

      break;
    case SADB_SATYPE_AH:
      /* auth_key */
      /* Get the first ah session key and encode the key */
      if (!crypto_map->ah_session_key)
        return -1;

      ah_session_key = listnode_head (crypto_map->ah_session_key);

      if (transform_set->ah_transform != TRANSFORM_AUTH_NONE)
        pal_mem_cpy (&vars->auth_key, ah_session_key->ah_auth_key, 
                     sizeof (vars->auth_key));

      /*auth_type */ 
      vars->auth_type = transform_set->ah_transform;
      vars->auth_len = ah_session_key->ah_auth_key_len;
      break;
    default:
      break;
    }

  return 0;
}

static int
_pal_keyadm_dump(Vars *vars)
{
  if(_pal_keyadm_send_command(vars, SADB_DUMP, vars->satype) == 0)
    {
      do
        {
          (void)pal_keyadm_receive_command(vars);
          (void)pal_keyadm_print_command(vars);

        }
      while((vars->sadb->msg->sadb_msg_errno == 0) &&
            (vars->sadb->msg->sadb_msg_seq));
    }

  return 0;
}

static int
_pal_keyadm_getspi(Vars *vars)
{
  if(vars->have_satype == 0)
    {
      zlog_info (nzg, "<keyadm> : missing modifier '-proto'. \n");
      return -1;
    }

  if(_pal_keyadm_send_command(vars, SADB_GETSPI, vars->satype) == 0)
    {
      (void)pal_keyadm_receive_command(vars);
      return pal_keyadm_print_command(vars);
    }

  return -1;
}

static int
_pal_keyadm_get(Vars *vars)
{
  if(vars->have_satype == 0)
    {
      zlog_info (nzg, "<keyadm> : missing modifier '-proto'.\n");
      return -1;
    }

  if(_pal_keyadm_send_command(vars, SADB_GET, vars->satype) == 0)
    {
      (void)pal_keyadm_receive_command(vars);
      return pal_keyadm_print_command(vars);
    }

  return -1;
}


static int
_pal_keyadm_update(Vars *vars)
{
  if(pal_keyadm_sacheck(vars) != 0)
    return -1;

  if(_pal_keyadm_send_command(vars, SADB_UPDATE, vars->satype) == 0)
    {
      (void)pal_keyadm_receive_command(vars);
      return pal_keyadm_print_command(vars);
    }

  return -1;
}

static int
_pal_keyadm_add(Vars *vars)
{
  if(pal_keyadm_sacheck(vars) != 0)
    return -1;

  if (_pal_keyadm_send_command (vars, SADB_ADD, vars->satype) == 0)
    {
      (void) pal_keyadm_receive_command (vars);
      return pal_keyadm_print_command(vars);
      return 0;
    }

  return -1;
}

int
_pal_keyadm_delete(Vars *vars)
{
  if ((vars->test == 0) && (vars->have_satype == 0))
    {
      zlog_info (nzg, "<keyadm> : missing modifier '-proto'. \n");
      return -1;
    }

  if (_pal_keyadm_send_command (vars, SADB_DELETE, vars->satype) == 0)
    {
      (void)pal_keyadm_receive_command(vars);
      pal_keyadm_print_command(vars);
      return 0;
    }

  return -1;
}

static int
_pal_keyadm_addflow(Vars *vars)
{
  if(_pal_keyadm_send_command(vars, SADB_X_ADDFLOW, vars->satype) == 0)
    {
      (void)pal_keyadm_receive_command(vars);
      return pal_keyadm_print_command(vars);
      return 0;
    }

  return -1;
}

static int
_pal_keyadm_delflow(Vars *vars)
{
  if(_pal_keyadm_send_command(vars, SADB_X_DELFLOW, vars->satype) == 0)
    {
      (void)pal_keyadm_receive_command(vars);
      pal_keyadm_print_command(vars);
      return 0;
    }

  return -1;
}

static int
_pal_keyadm_grpspis(Vars *vars)
{
  if(vars->test == 0)
    {
      if(vars->have_satype == 0)
        {
          zlog_info (nzg, "<keyadm> : missing modifier '-proto'. \n");
          return -1;
        }
      if(! CHECK_FLAG (vars->extmap, SEBIT_X_DST2))
        {
          zlog_info (nzg, "<keyadm> : missing modifier '-dst2'. \n");
          return -1;
        }
    }

  if(_pal_keyadm_send_command(vars, SADB_X_GRPSPIS, vars->satype) == 0)
    {
      (void)pal_keyadm_receive_command(vars);
      return pal_keyadm_print_command(vars);
      return 0;
    }

  return -1;
}

static int
_fill_grpspis (Vars *vars, struct ipsec_crypto_map *crypto_map1,
               struct ipsec_crypto_map *crypto_map2)
{
  struct ipsec_transform_set *transform_set1 = NULL;
  struct ipsec_transform_set *transform_set2 = NULL;
  struct ipsec_peer_addr *peer_addr = NULL;
  struct listnode *node = NULL;
  afi_t afi = AFI_IP;

  /* Memset data. */
  pal_mem_set (vars, 0, sizeof(Vars));
   /* prepare data */
  vars->sadb = (Keyadm_sadb *)vars->exthdr;
  vars->sadb->msg = &vars->sadbmsg;
  vars->encrypt_type = -1;
  vars->auth_type = -1;

  /* For not to print */
  vars->silent = 1;
  /* Have SA type. */
  vars->have_satype = 1;

  vars->extmap = SEBIT_SA;
   /* spi*/
  vars->spi_n = pal_hton32 (crypto_map1->spi);
  /* spi2 */
  vars->spi2_n = pal_hton32 (crypto_map2->spi);
  vars->extmap |= SEBIT_X_SA2;

  /* get the protocol */
  transform_set1 = listnode_head (crypto_map1->transform_set); 
  transform_set2 = listnode_head (crypto_map2->transform_set); 

  if (transform_set1->protocol == PROTO_AH)
    {
      vars->satype = SADB_SATYPE_AH;
    }
  else if (transform_set1->protocol == PROTO_ESP)
    {
      vars->satype =  SADB_SATYPE_ESP;
    }

  if (transform_set2->protocol == PROTO_AH)
    {
      vars->satype2 = SADB_SATYPE_AH;
    }
  else if (transform_set2->protocol == PROTO_ESP)
    {
      vars->satype2 =  SADB_SATYPE_ESP;
    }

  vars->extmap |= SEBIT_X_PROTOCOL;

  /* Destination address. */
  /* Get the first peer configured from the peer_list and add it */
  LIST_LOOP( crypto_map1->peer_addr, peer_addr, node)
    {
      if (afi == AFI_IP)
        {
          if (peer_addr->peer_addr.addr.family != AF_INET)
            continue;
          vars->dstaddr.sin.sin_len = sizeof (struct pal_sockaddr_in4);
          vars->dstaddr.sin.sin_family = AF_INET;
          vars->dstaddr.sin.sin_addr.s_addr = peer_addr->peer_addr.addr.prefix.s_addr;
        }
#ifdef HAVE_IPV6
      else if (afi == AFI_IP6)
        {
          if (peer_addr->peer_addr.ipv6_addr.family != AF_INET6)
            continue;

          vars->dstaddr.sin6.sin6_len = sizeof (struct pal_sockaddr_in6);
          vars->dstaddr.sin6.sin6_family = AF_INET6;
          pal_mem_cpy (&vars->dstaddr.sin6.sin6_addr, &peer_addr->peer_addr.ipv6_addr.prefix, sizeof (struct pal_in6_addr));
        }
#endif /* HAVE_IPV6 */

      vars->extmap |= SEBIT_ADDRESS_DST;
      break;
    }  
   /* Destination2 address. */
  /* Get the first peer configured from the peer_list and add it */
  LIST_LOOP( crypto_map2->peer_addr, peer_addr, node)
    {
      if (afi == AFI_IP)
        {
          if (peer_addr->peer_addr.addr.family != AF_INET)
            continue;
              vars->dstaddr2.sin.sin_len = sizeof (struct pal_sockaddr_in4);
              vars->dstaddr2.sin.sin_family = AF_INET;
              vars->dstaddr2.sin.sin_addr.s_addr = peer_addr->peer_addr.addr.prefix.s_addr;
        }
#ifdef HAVE_IPV6
      else if (afi == AFI_IP6)
        {
          if (peer_addr->peer_addr.ipv6_addr.family != AF_INET6)
            continue;

          vars->dstaddr2.sin6.sin6_len = sizeof (struct pal_sockaddr_in6);
          vars->dstaddr2.sin6.sin6_family = AF_INET6;
          pal_mem_cpy (&vars->dstaddr2.sin6.sin6_addr, &peer_addr->peer_addr.ipv6_addr.prefix, sizeof (struct pal_in6_addr));
        }
#endif /* HAVE_IPV6 */

      vars->extmap |= SEBIT_X_DST2;
      break;
    }

  return _pal_keyadm_grpspis (vars);
}

static char *
_isakmp_authentication_str (int auth_type)
{
  switch (auth_type)
    {
    case IPSEC_PRE_SHARE:
      return "pre_shared_key";
    case IPSEC_RSA_ENCR:
      return "dss_signature";
    case IPSEC_RSA_SIG:
      return "rsa_signature";
    default:
      return "pre_shared_key";
    }
}

static int
pal_ipsec_fill_vars (Vars *vars,
                     struct ipsec_crypto_map *crypto_map, 
                     struct interface *ifp, int cmd)
{
  struct access_list *al = NULL;
  struct filter_common *filter = NULL;
  struct filter_list *mfilter = NULL;
  struct listnode *node = NULL;
  struct ipsec_peer_addr *peer_addr = NULL;
  int ret = 0;
  union Ip_sockaddr_union tmp;
  char buf[30];
  struct apn_vr *vr;
  afi_t afi = AFI_IP;

  if (! vars || ! crypto_map || ! ifp)
    return 0;

  /* Memset data. */
  pal_mem_set (vars, 0, sizeof(Vars));

  /* Fill vars. */
  /* TBD, can't combine IPv4 and IPv6 crypto maps for now. 
     This is ugly. */
  LIST_LOOP( crypto_map->peer_addr, peer_addr, node)
    { 
      if (peer_addr->peer_addr.addr.family == AF_INET)
        {
          afi = AFI_IP;
          break;
        }
#ifdef HAVE_IPV6
      else if (peer_addr->peer_addr.addr.family == AF_INET6)
        {
          afi = AFI_IP6;
        }
#endif /* HAVE_IPV6 */
    }

  _ipsec_fill_vars (crypto_map, ifp, cmd, vars, afi);

  switch (cmd)
    {
    case SADB_GETSPI:
      ret = _pal_keyadm_getspi (vars);
      break;
    case SADB_UPDATE:
      ret = _pal_keyadm_update (vars);
      break;
    case SADB_ADD:
      vars->egress = 0;
      vars->ingress = 0;

      ret = _pal_keyadm_add (vars);

      vars->ingress = 1;
      vars->egress = 0;
      memcpy (&tmp, &vars->srcaddr, sizeof (vars->srcaddr));
      memcpy (&vars->srcaddr, &vars->dstaddr, sizeof (vars->srcaddr));
      memcpy (&vars->dstaddr, &tmp, sizeof (vars->srcaddr));

      ret = _pal_keyadm_add (vars);
      break;
    case SADB_DELETE:
      vars->egress = 0;
      vars->ingress = 0;

      ret = _pal_keyadm_delete (vars);

      vars->ingress = 1;
      vars->egress = 0;
      memcpy (&tmp, &vars->srcaddr, sizeof (vars->srcaddr));
      memcpy (&vars->srcaddr, &vars->dstaddr, sizeof (vars->srcaddr));
      memcpy (&vars->dstaddr, &tmp, sizeof (vars->srcaddr));

      ret = _pal_keyadm_delete (vars);
      break;
    case SADB_GET:
      ret = _pal_keyadm_get (vars);
    case SADB_ACQUIRE:
      break;
    case SADB_REGISTER:
      break;
    case SADB_EXPIRE:
      break;
    case SADB_FLUSH:
      ret = _pal_keyadm_flush (vars);
      break;
    case SADB_DUMP:
      ret = _pal_keyadm_dump (vars);
      break;
    case SADB_X_PROMISC: 
      break;
    case SADB_X_ADDFLOW:

      /* Forward direction. */
      vars->egress = 0;
      vars->ingress = 0;

      /* Only care for IPv4 flows until Anupam adds flow for IPv6. */
      if (afi == AFI_IP)
        {
          /* Convert access list to string for lookup. */
          pal_mem_set (buf, 0, sizeof (buf));
          snprintf (buf, sizeof (buf), "%d", crypto_map->accesslist_id);

          vr = apn_vr_get_privileged (nzg);
          if (vr == NULL)
            return -1;

          al = access_list_lookup (vr, AFI_IP, buf);
          if (al != NULL)
            {
              mfilter = al->head;
              filter = &mfilter->u.cfilter;
              if (filter->extended)
                {
                  /* Src mask. */
                  vars->srcmask.sin.sin_len = sizeof (struct pal_sockaddr_in4);
                  vars->srcmask.sin.sin_family = AF_INET;
                  vars->srcmask.sin.sin_addr.s_addr = ~filter->addr_mask.s_addr;
                  vars->extmap |= SEBIT_X_SRC_MASK;
                  struct pal_in4_addr addrval;
                  addrval.s_addr = 
                    pal_ntoh32 (vars->srcmask.sin.sin_addr.s_addr);
                  /* Dst mask. */
                  vars->dstmask.sin.sin_len = sizeof (struct pal_sockaddr_in4);
                  vars->dstmask.sin.sin_family = AF_INET;
                  vars->dstmask.sin.sin_addr.s_addr = ~filter->mask_mask.s_addr;
                  addrval.s_addr =
                    pal_ntoh32 (vars->dstmask.sin.sin_addr.s_addr);
                  vars->extmap |= SEBIT_X_DST_MASK;
          
                  /* Src flow. */
                  vars->srcflow.sin.sin_len = sizeof (struct pal_sockaddr_in4);
                  vars->srcflow.sin.sin_family = AF_INET;
                  vars->srcflow.sin.sin_addr.s_addr = filter->addr.s_addr;
                  vars->extmap |= SEBIT_X_SRC_FLOW;
          
                  /* Dst flow. */
                  vars->dstflow.sin.sin_len = sizeof (struct pal_sockaddr_in4);
                  vars->dstflow.sin.sin_family = AF_INET;
                  vars->dstflow.sin.sin_addr.s_addr = filter->mask.s_addr;
                  vars->extmap |= SEBIT_X_DST_FLOW;
                }
            }
        }
#ifdef HAVE_IPV6
      else if (afi == AFI_IP6)
        {
          struct pal_in6_addr netmask;
          struct pal_in6_addr src_netmask;
          struct filter_pacos_ext *z_ext_filter;
          vr = apn_vr_get_privileged (nzg);
          if (vr == NULL)
            return -1;

          al = access_list_lookup (vr, AFI_IP6, crypto_map->ipv6_acl_name);
          if (al != NULL)
            {
              /* For IPv6 all hosts/networks to all hosts/networks under SA treatment. */
              mfilter = al->head;
              z_ext_filter = &mfilter->u.zextfilter;

              pal_mem_set (&vars->srcmask, 0, sizeof (vars->srcmask));
              vars->srcmask.sin6.sin6_len = sizeof (struct pal_sockaddr_in6);
              vars->srcmask.sin6.sin6_family = AF_INET6;
              masklen2ip6 (z_ext_filter->sprefix.prefixlen, &src_netmask);
              pal_mem_cpy (&vars->srcmask.sin6.sin6_addr, &src_netmask,
                           sizeof (struct pal_in6_addr));
              vars->extmap |= SEBIT_X_SRC_MASK;
              
              /* Dst mask. */
              pal_mem_set (&vars->dstmask, 0, sizeof (vars->dstmask));
              vars->dstmask.sin6.sin6_len = sizeof (struct pal_sockaddr_in6);
              vars->dstmask.sin6.sin6_family = AF_INET6;
              masklen2ip6 (z_ext_filter->dprefix.prefixlen, &netmask);
              pal_mem_cpy (&vars->dstmask.sin6.sin6_addr, &netmask,
                           sizeof (struct pal_in6_addr));
              vars->extmap |= SEBIT_X_DST_MASK;
          
              /* Src flow. */
              pal_mem_set (&vars->srcflow, 0, sizeof (vars->srcflow));
              vars->srcflow.sin.sin_len = sizeof (struct pal_sockaddr_in6);
              vars->srcflow.sin.sin_family = AF_INET6;
              pal_mem_cpy (&vars->srcflow.sin6.sin6_addr,
                           &z_ext_filter->sprefix.u.prefix6,
                           sizeof (struct pal_in6_addr));

              vars->extmap |= SEBIT_X_SRC_FLOW;
          
              /* Dst flow. */
              pal_mem_set (&vars->dstflow, 0, sizeof (vars->dstflow));
              vars->dstflow.sin.sin_len = sizeof (struct pal_sockaddr_in6);
              vars->dstflow.sin.sin_family = AF_INET6;
              pal_mem_cpy (&vars->dstflow.sin6.sin6_addr,
                           &z_ext_filter->sprefix.u.prefix6,
                           sizeof (struct pal_in6_addr));
              vars->extmap |= SEBIT_X_DST_FLOW;
            }
        }
#endif /* HAVE_IPV6 */
      
      ret = _pal_keyadm_addflow (vars);

      /* Reverse direction. */
      vars->egress = 0;
      vars->ingress = 1;

      memcpy (&tmp, &vars->srcaddr, sizeof (vars->srcaddr));
      memcpy (&vars->srcaddr, &vars->dstaddr, sizeof (vars->srcaddr));
      memcpy (&vars->dstaddr, &tmp, sizeof (vars->srcaddr));

      memcpy (&tmp, &vars->srcmask, sizeof (vars->srcmask));
      memcpy (&vars->srcmask, &vars->dstmask, sizeof (vars->srcmask));
      memcpy (&vars->dstmask, &tmp, sizeof (vars->srcmask));

      memcpy (&tmp, &vars->srcflow, sizeof (vars->srcflow));
      memcpy (&vars->srcflow, &vars->dstflow, sizeof (vars->srcflow));
      memcpy (&vars->dstflow, &tmp, sizeof (vars->srcflow));

      ret = _pal_keyadm_addflow (vars);

      break;
    case SADB_X_DELFLOW:

       /* Forward direction. */
      vars->egress = 0;
      vars->ingress = 0;
      UNSET_FLAG (vars->extmap, SEBIT_ADDRESS_SRC);
      /* Only care for IPv4 flows until Anupam adds flow for IPv6. */
      if (afi == AFI_IP)
        {
          /* Convert access list to string for lookup. */
          pal_mem_set (buf, 0, sizeof (buf));
          snprintf (buf, sizeof (buf), "%d", crypto_map->accesslist_id);

          vr = apn_vr_get_privileged (nzg);
          if (vr == NULL)
            return -1;

          al = access_list_lookup (vr, AFI_IP, buf);
          if (al != NULL)
            {
              mfilter = al->head;
              filter = &mfilter->u.cfilter;
              if (filter->extended)
                {
                  /* Src mask. */
                  vars->srcmask.sin.sin_len = sizeof (struct pal_sockaddr_in4);
                  vars->srcmask.sin.sin_family = AF_INET;
                  vars->srcmask.sin.sin_addr.s_addr = ~filter->addr_mask.s_addr;
                  vars->extmap |= SEBIT_X_SRC_MASK;
                  struct pal_in4_addr addrval;
                  addrval.s_addr =
                    pal_ntoh32 (vars->srcmask.sin.sin_addr.s_addr);
                  /* Dst mask. */
                  vars->dstmask.sin.sin_len = sizeof (struct pal_sockaddr_in4);
                  vars->dstmask.sin.sin_family = AF_INET;
                  vars->dstmask.sin.sin_addr.s_addr = ~filter->mask_mask.s_addr;
                  addrval.s_addr =
                    pal_ntoh32 (vars->dstmask.sin.sin_addr.s_addr);
                  vars->extmap |= SEBIT_X_DST_MASK;

                  /* Src flow. */
                  vars->srcflow.sin.sin_len = sizeof (struct pal_sockaddr_in4);
                  vars->srcflow.sin.sin_family = AF_INET;
                  vars->srcflow.sin.sin_addr.s_addr = filter->addr.s_addr;
                  vars->extmap |= SEBIT_X_SRC_FLOW;

                  /* Dst flow. */
                  vars->dstflow.sin.sin_len = sizeof (struct pal_sockaddr_in4);
                  vars->dstflow.sin.sin_family = AF_INET;
                  vars->dstflow.sin.sin_addr.s_addr = filter->mask.s_addr;
                  vars->extmap |= SEBIT_X_DST_FLOW;
                }
            }
        }
#ifdef HAVE_IPV6
      else if (afi == AFI_IP6)
        {
          struct pal_in6_addr netmask;
          struct pal_in6_addr src_netmask;
          struct filter_pacos_ext *z_ext_filter;
          vr = apn_vr_get_privileged (nzg);
          if (vr == NULL)
            return -1;

          al = access_list_lookup (vr, AFI_IP6, crypto_map->ipv6_acl_name);
          if (al != NULL)
            {
              /* For IPv6 all hosts/networks to all hosts/networks under SA treatment. */
              mfilter = al->head;
              z_ext_filter = &mfilter->u.zextfilter;

              pal_mem_set (&vars->srcmask, 0, sizeof (vars->srcmask));
              vars->srcmask.sin6.sin6_len = sizeof (struct pal_sockaddr_in6);
              vars->srcmask.sin6.sin6_family = AF_INET6;
              masklen2ip6 (z_ext_filter->sprefix.prefixlen, &src_netmask);
              pal_mem_cpy (&vars->srcmask.sin6.sin6_addr, &src_netmask,
                           sizeof (struct pal_in6_addr));
              vars->extmap |= SEBIT_X_SRC_MASK;

              /* Dst mask. */
              pal_mem_set (&vars->dstmask, 0, sizeof (vars->dstmask));
              vars->dstmask.sin6.sin6_len = sizeof (struct pal_sockaddr_in6);
              vars->dstmask.sin6.sin6_family = AF_INET6;
              masklen2ip6 (z_ext_filter->dprefix.prefixlen, &netmask);
              pal_mem_cpy (&vars->dstmask.sin6.sin6_addr, &netmask,
                           sizeof (struct pal_in6_addr));
              vars->extmap |= SEBIT_X_DST_MASK;

              /* Src flow. */
              pal_mem_set (&vars->srcflow, 0, sizeof (vars->srcflow));
              vars->srcflow.sin.sin_len = sizeof (struct pal_sockaddr_in6);
              vars->srcflow.sin.sin_family = AF_INET6;
              pal_mem_cpy (&vars->srcflow.sin6.sin6_addr,
                           &z_ext_filter->sprefix.u.prefix6,
                           sizeof (struct pal_in6_addr));

              /* Dst flow. */
              pal_mem_set (&vars->dstflow, 0, sizeof (vars->dstflow));
              vars->dstflow.sin.sin_len = sizeof (struct pal_sockaddr_in6);
              vars->dstflow.sin.sin_family = AF_INET6;
              pal_mem_cpy (&vars->dstflow.sin6.sin6_addr,
                           &z_ext_filter->sprefix.u.prefix6,
                           sizeof (struct pal_in6_addr));
              vars->extmap |= SEBIT_X_DST_FLOW;
            }
        }
#endif /* HAVE_IPV6 */

      ret = _pal_keyadm_delflow (vars);

      /* Reverse direction. */
      vars->egress = 0;
      vars->ingress = 1;

      memcpy (&tmp, &vars->srcaddr, sizeof (vars->srcaddr));
      memcpy (&vars->dstaddr, &tmp, sizeof (vars->srcaddr));

      memcpy (&tmp, &vars->srcmask, sizeof (vars->srcmask));
      memcpy (&vars->srcmask, &vars->dstmask, sizeof (vars->srcmask));
      memcpy (&vars->dstmask, &tmp, sizeof (vars->srcmask));

      memcpy (&tmp, &vars->srcflow, sizeof (vars->srcflow));
      memcpy (&vars->srcflow, &vars->dstflow, sizeof (vars->srcflow));
      memcpy (&vars->dstflow, &tmp, sizeof (vars->srcflow));

      ret = _pal_keyadm_delflow (vars);

      break;
    case SADB_X_GRPSPIS:
      ret = _pal_keyadm_grpspis (vars);
      break;
    case SADB_X_BINDSA:
      break;
    default:
      break;
    }
  return ret;
}
/* Api calls from the nsm which inturn call the static apis declared 
   in this file */

/* Api to add a manual SA */
int
pal_ipsec_add_sa (struct ipsec_crypto_map_bundle *crypto_bundle,
                  struct interface *ifp)
{
  struct ipsec_crypto_map *crypto_map2 = NULL;
  struct ipsec_crypto_map *crypto_map = NULL;
  struct interface *tmp_ifp = NULL;
  struct listnode *node = NULL;
  int ret = -1;
  int count = 0;
  Vars vars;

  /* We will use the local address if configured */
  if (CHECK_FLAG (crypto_bundle->flags, IPSEC_CRYPTO_BUNDLE_LOCAL_ADDR))
    {
      tmp_ifp = ifg_lookup_by_name (&nzg->ifg,
                                    crypto_bundle->local_addr_ifname); 
      if (tmp_ifp)
        ifp = tmp_ifp;
    }

  LIST_LOOP (crypto_bundle->crypto_map, crypto_map, node)
    if (crypto_map && ifp)
      ret = pal_ipsec_fill_vars (&vars, crypto_map, ifp, SADB_ADD);

  if (!ret)
    ret = pal_ipsec_add_sa_flow (crypto_bundle, ifp);

  if (listcount (crypto_bundle->crypto_map) > 1)
    {
      LIST_LOOP (crypto_bundle->crypto_map, crypto_map, node)
      {
        if (crypto_map && ifp)
          {
            count++;
            if (count % 2 == 0)
              ret = _fill_grpspis (&vars, crypto_map2, crypto_map);
            else
              crypto_map2 = crypto_map;
          }
      }
    }

  return ret;
}
/* Api to add flow to SA */
int
pal_ipsec_add_sa_flow (struct ipsec_crypto_map_bundle *crypto_bundle,
                       struct interface *ifp)
{
  struct ipsec_crypto_map *crypto_map = NULL;
  struct interface *tmp_ifp = NULL;
  struct listnode *node = NULL;
  Vars vars;
  int ret = -1;

  /* We will use the local address if configured */
  if (CHECK_FLAG (crypto_bundle->flags, IPSEC_CRYPTO_BUNDLE_LOCAL_ADDR))
    {
      tmp_ifp = ifg_lookup_by_name (&nzg->ifg,
                                    crypto_bundle->local_addr_ifname);
      if (tmp_ifp)
        ifp = tmp_ifp;
    }

  LIST_LOOP (crypto_bundle->crypto_map, crypto_map, node)
    if (crypto_map && ifp)
      ret = pal_ipsec_fill_vars (&vars, crypto_map, ifp, SADB_X_ADDFLOW);

  return ret;
}
/* Api to add flow to SA */
int
pal_ipsec_del_sa_flow (struct ipsec_crypto_map_bundle *crypto_bundle,
                       struct interface *ifp)
{
  struct ipsec_crypto_map *crypto_map = NULL;
  struct interface *tmp_ifp = NULL;
  struct listnode *node = NULL;
  Vars vars;
  int ret = -1;

  /* We will use the local address if configured */
  if (CHECK_FLAG (crypto_bundle->flags, IPSEC_CRYPTO_BUNDLE_LOCAL_ADDR))
    {
      tmp_ifp = ifg_lookup_by_name (&nzg->ifg,
                                    crypto_bundle->local_addr_ifname);
      if (tmp_ifp)
        ifp = tmp_ifp;
    }
  LIST_LOOP (crypto_bundle->crypto_map, crypto_map, node)
    if (crypto_map && ifp)
      ret = pal_ipsec_fill_vars (&vars, crypto_map, ifp, SADB_X_DELFLOW);

  return ret;
}
/* Api to Delete a manual SA */
int
pal_ipsec_delete_sa (struct ipsec_crypto_map_bundle *crypto_bundle, 
                     struct interface *ifp)
{
  struct ipsec_crypto_map *crypto_map = NULL;
  struct interface *tmp_ifp = NULL;
  struct listnode *node = NULL;
  Vars vars;
  int ret = -1;

  /* We will use the local address if configured */
  if (CHECK_FLAG (crypto_bundle->flags, IPSEC_CRYPTO_BUNDLE_LOCAL_ADDR))
    {
      tmp_ifp = ifg_lookup_by_name (&nzg->ifg,
                                    crypto_bundle->local_addr_ifname);
      if (tmp_ifp)
        ifp = tmp_ifp;
    }

  LIST_LOOP (crypto_bundle->crypto_map, crypto_map, node)
    if (crypto_map && ifp)
      ret = pal_ipsec_fill_vars (&vars, crypto_map, ifp, SADB_DELETE);

  return ret;
}

/* Static funstion to fill the nasm sadb data structure */
static int
_fill_nsm_sadb_vars (struct nsm_sadb *sadb, Vars *vars)
{
  union Ip_sockaddr_union   *sockaddr;
  int   addr_offset;

  sadb->sadb_sa_spi = vars->sadb->sa->sadb_sa_spi;
  sadb->sadb_sa_state = vars->sadb->sa->sadb_sa_state;
  sadb->sadb_sa_auth = vars->sadb->sa->sadb_sa_auth;
  sadb->sadb_sa_encrypt = vars->sadb->sa->sadb_sa_encrypt;
  sadb->sadb_sa_replay = (int)vars->sadb->sa->sadb_sa_replay;
  sadb->sadb_sa_flags = vars->sadb->sa->sadb_sa_flags;

  if (vars->sadb->address_src)
    {
      sockaddr = (union Ip_sockaddr_union *) \
        ((u_int8_t *)vars->sadb->address_src + sizeof(struct sadb_address));
      addr_offset = (sockaddr->sa.sa_family == AF_INET) ? 2 : 6;
      pal_inet_ntop(sockaddr->sa.sa_family,
                             &sockaddr->sa.sa_data[addr_offset],
                             sadb->src_addr_str, sizeof(sadb->src_addr_str));
    }

  if (vars->sadb->address_dst)
    {
      sockaddr = (union Ip_sockaddr_union *) \
        ((u_int8_t *)vars->sadb->address_dst + sizeof(struct sadb_address));
      addr_offset = (sockaddr->sa.sa_family == AF_INET) ? 2 : 6;

      pal_inet_ntop(sockaddr->sa.sa_family,
                    &sockaddr->sa.sa_data[addr_offset],
                    sadb->dst_addr_str, sizeof(sadb->dst_addr_str));
    }

  if (vars->sadb->address_proxy)
    {
      sockaddr = (union Ip_sockaddr_union *)((u_int8_t *) \
                  vars->sadb->address_proxy + sizeof(struct sadb_address));
      addr_offset = (sockaddr->sa.sa_family == AF_INET) ? 2 : 6;

      pal_inet_ntop(sockaddr->sa.sa_family,
                    &sockaddr->sa.sa_data[addr_offset],
                    sadb->proxy_addr_str, sizeof (sadb->proxy_addr_str));
    }
  /*Populate the spi range field */
  if (vars->sadb->spirange)
    {
      sadb->sadb_spirange_min = vars->sadb->spirange->sadb_spirange_min;
      sadb->sadb_spirange_max = vars->sadb->spirange->sadb_spirange_max;
    }

  /* Populate the current lifetime */
  if (vars->sadb->lifetime_current)
    {
      sadb->current_lifetime.addtime = (time_t) IP_U64_GETLO \
        (vars->sadb->lifetime_current->sadb_lifetime_addtime);
      sadb->current_lifetime.usetime = (time_t) IP_U64_GETLO \
        (vars->sadb->lifetime_current->sadb_lifetime_usetime);
      sadb->current_lifetime.bytes = IP_U64_GETLO \
        (vars->sadb->lifetime_current->sadb_lifetime_bytes);
    }

  /* Populate the soft time */
  if (vars->sadb->lifetime_soft)
    {
      sadb->soft_lifetime.addtime = (time_t) IP_U64_GETLO \
        (vars->sadb->lifetime_soft->sadb_lifetime_addtime);
      sadb->soft_lifetime.usetime = (time_t) IP_U64_GETLO \
        (vars->sadb->lifetime_soft->sadb_lifetime_usetime);
      sadb->soft_lifetime.bytes   = IP_U64_GETLO \
        (vars->sadb->lifetime_soft->sadb_lifetime_bytes);
    }

  /* Populate the hard time */
  if (vars->sadb->lifetime_hard)
    {
      sadb->hard_lifetime.addtime = (time_t) IP_U64_GETLO \
        (vars->sadb->lifetime_hard->sadb_lifetime_addtime);
      sadb->hard_lifetime.usetime = (time_t) IP_U64_GETLO \
        (vars->sadb->lifetime_hard->sadb_lifetime_usetime);
      sadb->hard_lifetime.bytes   = IP_U64_GETLO \
        (vars->sadb->lifetime_hard->sadb_lifetime_bytes);
    }

  return 0;
}

/* API to get a SA */
int
pal_ipsec_get_sas (struct nsm_sadb *sadb, struct ipsec_crypto_map *cryptomap,
                   struct interface *ifp)
                   
{
  Vars vars;
  int ret = -1;
  /* for time being we keep the things simple. Later we need to have some 
     similar structure in nsm and pass that variable with some options and 
     get the SA information back to nsm where we will print in the cli */
  if (cryptomap && ifp)
    ret = pal_ipsec_fill_vars (&vars, cryptomap, ifp, SADB_GET);

  if (!ret)
    {
      _fill_nsm_sadb_vars (sadb, &vars);
    }

  return ret;
}

/*API to flush a particular SA .DUMMY now*/
int
pal_ipsec_flush_sa (struct ipsec_crypto_map_bundle *crypto_bundle,
                    struct interface *ifp)
{
  /* We will call the delete sa function to flush a particular sa */
  return pal_ipsec_delete_sa (crypto_bundle, ifp);
}

static char *
_isakmp_hash_algo_str (int hash_algo)
{
  switch (hash_algo)
    {
    case IPSEC_AUTH_MD5_HMAC:
      return "md5";
    case IPSEC_AUTH_SHA1_HMAC:
      return "sha1";
    default:
      return "md5";
    }
}

static char *
_isakmp_auth_algo_str (int auth_algo)
{
  switch (auth_algo)
    {
    case TRANSFORM_AUTH_MD5:
      return "md5";
    case TRANSFORM_AUTH_SHA1:
      return "sha1";
    default:
      return "md5"; 
    }
}

static char *
_isakmp_enc_algo_str (int enc_algo)
{
  switch (enc_algo)
    {
    default:
    case TRANSFORM_ESP_ENC_NONE:
    case TRANSFORM_ESP_ENC_DES:
      return "des";
    case TRANSFORM_ESP_ENC_3DES:
      return "3des";
    case TRANSFORM_ESP_ENC_BLOWFISH:
      return "blowfish";
    case TRANSFORM_ESP_ENC_CAST:
      return "cast";
    case TRANSFORM_ESP_ENC_AES:
      return "aes128";
    }
}

static int
_pal_restart_ike (void)
{
  int pid1, pid2;
  int ret1, ret2;

  pid1 = fork ();
  switch (pid1)
   {
     case -1:
       exit (1);
     case 0:
       /* Child1. */

       pid2 = fork ();
       switch (pid2)
         {
           case -1:
             exit (1);
           case 0:
             /* Child2. */
             system ("ipiked -d");
             exit (0);
           default:
             wait (&ret2);
         }
       exit (0); 
     default:
       /* Parent. */
   
       wait (&ret1); 
   }

  return 0;
}

static int
_pal_reconfig_ike (void)
{
  FILE *fp;
  char buf[10];
  int pid;

  /* Find PID of ipiked. */
  system ("ps -e|grep ipiked |awk '{print $1}' > /tmp/ipiked.pid");

  pid = -1;

  fp = fopen ("/tmp/ipiked.pid", "r");
  if (fp == NULL)
    goto RESTART;

  if (fgets (buf, sizeof (buf), fp) != NULL)
    pid = atoi (buf);

  fclose (fp);

  if (pid > 0)
      kill (pid, SIGHUP);

  return 0;

 RESTART:

  _pal_restart_ike ();

  return 0;
}

/* Defines for tags. */
#define GENERAL_TAG                   "general"
#define LISTEN_TAG                    "listen "
#define NEWLINE                       "\n"
#define OPEN_TAG                      "{"
#define CLOSE_TAG                     "}"
#define IKESA_ADDR_TAG                "ike_sa address "
#define LOCAL_ADDRESS                 "local_address "
#define PASSIVE_TAG                   "passive off;"
#define AUTHENTICATION_TAG            "authentication "
#define AUTH_METHOD_TAG               "method "
#define SHARED_KEY_TAG                "shared_key  "
#define PROPOSAL_TAG                  "proposal "
#define TERMINATOR_TAG                ";"
#define SEPERATOR_TAG                 ","
#define DH_GROUP_TAG                  "dh_group "
#define HASH_ALGO_TAG                 "hash_algorithm "
#define INTEGRITY_ALGO_TAG            "integrity_algorithm "
#define IPSEC_TAG                     "ipsec_sa "
#define IKE_ID_TAG                    "ike_id "
#define ENCRYPT_ALGO_TAG              "encryption_algorithm "
#define INTEGRITY_ALGO_TAG            "integrity_algorithm "
#define IP_ADDRESS_TAG                "address "
#define LIFETIME                      "lifetime "
#define SECS                          "seconds"
#define KBYTES                        "kilobytes"
#define LIVENESS                      "liveness "

#define SPACE_TAG                     " "

#define PUT_IKEID(buf,num,fp)                                                    \
   do {                                                                          \
     zsnprintf (buf, sizeof (buf), "%s %d %s",                                   \
                IKE_ID_TAG, num, TERMINATOR_TAG);                                \
                fputs (buf, fp);                                                 \
                fputs (NEWLINE, fp);                                             \
   } while (0)

#define PUT_DHGROUP(buf,isakmp,fp)                                               \
   do {                                                                          \
     zsnprintf (buf, sizeof (buf), "%s %d %s",                                   \
                DH_GROUP_TAG, isakmp->group, TERMINATOR_TAG);                    \
                fputs (buf, fp);                                                 \
                fputs (NEWLINE, fp);                                             \
   } while (0)

#define PUT_ENC_ALGO(first,atleastone,cmap,tset,node3,tstr)                      \
   do {                                                                          \
     first = 1;                                                                  \
     atleastone = 0;                                                             \
     /* Encryption for all transform sets. */                                    \
     LIST_LOOP (cmap->transform_set, tset, node3)                                \
       {                                                                         \
         if (tset->protocol == PROTO_ESP)                                        \
           {                                                                     \
             atleastone = 1;                                                     \
                                                                                 \
             if (first)                                                          \
               {                                                                 \
                fputs (ENCRYPT_ALGO_TAG, fp);                                    \
                first = 0;                                                       \
               }                                                                 \
                                                                                 \
             tstr = _isakmp_enc_algo_str (tset->esp_enc_transform);               \
                                                                                 \
             fputs (tstr, fp);                                                   \
             if (node3->next != NULL)                                            \
               fputs (SEPERATOR_TAG, fp);                                        \
           }                                                                     \
        }                                                                        \
        if (atleastone)                                                          \
         {                                                                       \
           fputs (TERMINATOR_TAG, fp);                                           \
           fputs (NEWLINE, fp);                                                  \
         }                                                                       \
   } while (0)

#define PUT_INTEGRITY_ALGO(first,atleastone,cmap,tset,node3,tstr)                \
   do {                                                                          \
     first = 1;                                                                  \
     atleastone = 0;                                                             \
     /* Integrity algorithm for all transform sets. */                           \
     LIST_LOOP (cmap->transform_set, tset, node3)                                \
       {                                                                         \
         atleastone = 1;                                                         \
                                                                                 \
         if (first)                                                              \
           {                                                                     \
             fputs (INTEGRITY_ALGO_TAG, fp);                                     \
             first = 0;                                                          \
           }                                                                     \
                                                                                 \
         if (tset->protocol == PROTO_ESP)                                        \
           {                                                                     \
             tstr = _isakmp_auth_algo_str (tset->esp_auth_transform);             \
           }                                                                     \
         else                                                                    \
           {                                                                     \
             tstr = _isakmp_auth_algo_str (tset->ah_transform);                   \
           }                                                                     \
         fputs (tstr, fp);                                                       \
         if (node3->next != NULL)                                                \
            fputs (SEPERATOR_TAG, fp);                                           \
       }                                                                         \
     if (atleastone)                                                             \
       {                                                                         \
         fputs (TERMINATOR_TAG, fp);                                             \
         fputs (NEWLINE, fp);                                                    \
       }                                                                         \
  } while (0)

int
pal_ipsec_ike_config (struct ipsec_crypto_map_bundle *crypto_bundle)
{
  struct listnode *node1, *node2, *node3, *node4, *node5;
  struct apn_vr *vr = apn_vr_get_privileged (nzg);
  struct ipsec_crypto_isakmp *isakmp = NULL;
  struct nsm_ipsec_master *ipsec_master;
  struct ipsec_peer_addr *peer_addr1;
  struct connected *conn_addr = NULL;
  struct ipsec_transform_set *tset;
  struct ipsec_peer_addr *tmp_addr;
  struct ipsec_crypto_map *cmap;
  struct interface *ifp = NULL;
  struct filter_common *filter;
  struct filter_list *mfilter;
  struct prefix listen_addr;
  struct pal_in4_addr addr;
  struct interface *tifp;
  struct access_list *al;
  struct connected *ifc;
  int first, atleastone;
  struct nsm_master *nm;
  struct list *tlist;
  struct prefix *p;
  struct prefix q;
  char buf[4096];
  char *tstr;
  afi_t afi = AFI_IP;
  FILE *fp;
  int num;

  /* Sanity Check */
  if (vr == NULL)
    {
      zlog_info (nzg, "VR not found \n");
      return -1;
    }

  nm = nsm_master_lookup_by_id (nzg, vr->id);
  if (nm == NULL)
    {
      zlog_info (nzg, "NSM master not found \n");
      return -1;
    }

  ipsec_master = nm->ipsec_master;

  num = 1;

  tlist = list_new ();

  if (tlist == NULL)
    return -1;

  fp = fopen (IPSEC_DEFAULT_FILENAME, "w+");
  if (fp == NULL)
    return -1;

  fputs (GENERAL_TAG, fp);
  fputs (OPEN_TAG, fp);
  fputs (NEWLINE, fp);
  fputs (LISTEN_TAG, fp);
  pal_mem_set (&listen_addr, 0, sizeof (struct prefix));

  /* General section. */
  LIST_LOOP (crypto_bundle->crypto_map, cmap, node1)
    {
      LIST_LOOP (cmap->peer_addr, tmp_addr, node2)
      {
        if (tmp_addr->peer_addr.addr.family == AF_INET)
          {
            afi = AFI_IP;
            break;
          }
        else
          {
            afi = AFI_IP6;
             break;
          }
      }
    }

  /* Check if listener is configured use that one */
  if (CHECK_FLAG (crypto_bundle->flags, IPSEC_CRYPTO_BUNDLE_LOCAL_ADDR))
    {
      ifp = ifg_lookup_by_name (&nzg->ifg, crypto_bundle->local_addr_ifname);
      if (ifp)
        {
          if (afi == AFI_IP)
            {
              for (conn_addr = ifp->ifc_ipv4; conn_addr; 
                                               conn_addr = conn_addr->next)
                 if (conn_addr->address
                     && ! CHECK_FLAG (conn_addr->flags, NSM_IFA_SECONDARY))
                   {
                     prefix_copy (&listen_addr, conn_addr->address);
                     pal_inet_ntop (AF_INET, &listen_addr.u.prefix4, buf,
                                 sizeof (buf));
                     fputs (buf, fp);
                      break;
                    }
             }
#ifdef HAVE_IPV6
            else
             {
               for (conn_addr = ifp->ifc_ipv6; conn_addr;
                                               conn_addr = conn_addr->next)
                 if (conn_addr->address
                     && ! CHECK_FLAG (conn_addr->flags, NSM_IFA_SECONDARY))
                   {
                     prefix_copy (&listen_addr, conn_addr->address);
                     pal_inet_ntop (AF_INET, &listen_addr.u.prefix6, buf,
                                  sizeof (buf));
                     fputs (buf, fp);
                     break;
                   }
             }
#endif /* HAVE_IPV6 */
        }
    }
  else
  /* First add all local interfaces as listeners. 
     IPv4 for now. */
     LIST_LOOP (crypto_bundle->if_list, tifp, node2)
     {
       if (afi == AFI_IP)
         {
           for (ifc = tifp->ifc_ipv4; ifc; ifc = ifc->next)
             {
               /* Only promary addresses for now. */
               if (CHECK_FLAG (ifc->conf, NSM_IFC_REAL) && 
                   ! CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY) &&
                   ! CHECK_FLAG (ifc->flags, NSM_IFA_ANYCAST) &&
                   ! CHECK_FLAG (ifc->flags, NSM_IFA_VIRTUAL) &&
                   ! listnode_lookup (tlist, ifc))
                 {
                   p = ifc->address; 
                   prefix_copy (&listen_addr, (struct prefix *)p);
                   /* Convert to address format. */
                   pal_inet_ntop (AF_INET, &p->u.prefix4, buf, sizeof (buf));
                   listnode_add (tlist, ifc); 
                   fputs (buf, fp);
                   if (ifc->next != NULL)
                     fputs (SEPERATOR_TAG, fp);
                 }
            }
        }
#ifdef HAVE_IPV6
      else
        {
          for (ifc = tifp->ifc_ipv6; ifc; ifc = ifc->next)
            {
               p = ifc->address; 

               if (! IN6_IS_ADDR_LINKLOCAL (&p->u.prefix6) &&
                   ! listnode_lookup (tlist, ifc))
                 {
                   /* Convert to address format. */
                   pal_inet_ntop (AF_INET6, &p->u.prefix6, buf, sizeof (buf));
                   listnode_add (tlist, ifc);
                   prefix_copy (&listen_addr, (struct prefix *)p);
                   fputs (buf, fp);
        
                   if (ifc->next != NULL)
                     fputs (SEPERATOR_TAG, fp);       
                }
            }
        }
#endif /* HAVE_IPV6 */
     }
  

  fputs (TERMINATOR_TAG, fp);
  fputs (NEWLINE, fp);
  fputs (CLOSE_TAG, fp);
  fputs (NEWLINE, fp);

  LIST_LOOP (ipsec_master->ipsec_crypto_isakmp_list, isakmp, node2)
   {
     if (isakmp->peer_addr.addr.family == AF_INET)
       afi = AFI_IP;
     else
       afi = AFI_IP6;

     if (afi == AFI_IP)
       zsnprintf (buf, sizeof (buf), "%s%r ", 
                  IKESA_ADDR_TAG,
                  &isakmp->peer_addr.addr.prefix);
#ifdef HAVE_IPV6
     else
       zsnprintf (buf, sizeof (buf), "%s%R ", 
                  IKESA_ADDR_TAG,
                  &isakmp->peer_addr.ipv6_addr.prefix);
#endif /* HAVE_IPV6 */

     fputs (buf, fp);
     /* Open tag. */
     fputs (OPEN_TAG, fp);
     fputs (NEWLINE, fp);

     /* ID */
     zsnprintf (buf, sizeof (buf), "%s %d %s", IKE_ID_TAG, 
                num, TERMINATOR_TAG);
     fputs (buf, fp);
     fputs (NEWLINE, fp);

     /* Local address */
     /* For now we will have the default i.e the first listening ip 
        address */
     if (listen_addr.family == AF_INET)
       zsnprintf (buf, sizeof (buf), "%s %r %s",
                  LOCAL_ADDRESS, &listen_addr.u.prefix4, TERMINATOR_TAG);
#ifdef HAVE_IPV6
     else
       zsnprintf (buf, sizeof (buf), "%s %R %s",
                  LOCAL_ADDRESS, &listen_addr.u.prefix6, TERMINATOR_TAG);
#endif /* HAVE_IPV6 */

     fputs (buf, fp);
     fputs (NEWLINE, fp);
          
          
     /* Passive. */
     fputs (PASSIVE_TAG, fp);
     fputs (NEWLINE, fp);

     /* Liveness */
     if (ipsec_master->keepalive_secs)
       {
         zsnprintf (buf, sizeof (buf), "%s %d %s",
                    LIVENESS, ipsec_master->keepalive_secs ,TERMINATOR_TAG);
         fputs (buf, fp);
         fputs (NEWLINE, fp);
       }

     /* Lifetime */
     if (isakmp->lifetime != 28800)
       {
         zsnprintf (buf, sizeof (buf), "%s %d",
                    LIFETIME, isakmp->lifetime);
         fputs (buf, fp);
         fputs (NEWLINE, fp);
       }

     /* Authentication. */
     fputs (AUTHENTICATION_TAG, fp);
     fputs (OPEN_TAG, fp);
     fputs (NEWLINE, fp);

     /* Authentication method. */
     zsnprintf (buf, sizeof (buf), "%s %s %s",
                AUTH_METHOD_TAG, 
                _isakmp_authentication_str (isakmp->authentication),
                TERMINATOR_TAG);
     fputs (buf, fp);
     fputs (NEWLINE, fp);

     /* Shared key. */
     zsnprintf (buf, sizeof (buf), "%s %s %s\n",
                SHARED_KEY_TAG, isakmp->key, TERMINATOR_TAG);

     fputs (buf, fp);
     fputs (NEWLINE, fp);
     fputs (CLOSE_TAG, fp);
     fputs (NEWLINE, fp);

     /* Proposal section. */
     fputs (PROPOSAL_TAG, fp);

     /* Open tag. */
     fputs (OPEN_TAG, fp);
     fputs (NEWLINE, fp);

     /* Group. */
     zsnprintf (buf, sizeof (buf), "%s %d %s", 
                DH_GROUP_TAG, isakmp->group, TERMINATOR_TAG);
     fputs (buf, fp);
     fputs (NEWLINE, fp);

     /* Hash algorithm. */
     zsnprintf (buf, sizeof (buf), "%s %s %s",
                HASH_ALGO_TAG, _isakmp_hash_algo_str (isakmp->hash_algo),
                TERMINATOR_TAG);
     fputs (buf, fp);
     fputs (NEWLINE, fp);

     /* Encryption algorithm. */
     zsnprintf (buf, sizeof (buf), "%s %s %s",
                ENCRYPT_ALGO_TAG, 
                _isakmp_enc_algo_str (isakmp->encrypt_algo),
                TERMINATOR_TAG);
     fputs (buf, fp);
     fputs (NEWLINE, fp);
                    
     /* Integrity algorithm. */
     zsnprintf (buf, sizeof (buf), "%s %s %s",
                INTEGRITY_ALGO_TAG, 
                _isakmp_hash_algo_str (isakmp->hash_algo),
                TERMINATOR_TAG);
     fputs (buf, fp);
     fputs (NEWLINE, fp);
          
     /* Close tag. */ 
     fputs (CLOSE_TAG, fp);
     fputs (NEWLINE, fp);
         
     /* Close IKE SA tag. */ 
     fputs (CLOSE_TAG, fp);
     fputs (NEWLINE, fp);
  }

  LIST_LOOP (crypto_bundle->crypto_map, cmap, node2)
  {
    LIST_LOOP (cmap->peer_addr, peer_addr1, node4)
      LIST_LOOP (ipsec_master->ipsec_crypto_isakmp_list, isakmp, node5)
       if (prefix_same ((struct prefix *)&isakmp->peer_addr.addr, 
                        (struct prefix *) &peer_addr1->peer_addr.addr))
         break;
    if (isakmp == NULL)
      isakmp = listnode_head (ipsec_master->ipsec_crypto_isakmp_list);

    if (isakmp == NULL)
      goto ERR;

    if (cmap->transform_set == NULL)
      goto ERR;

    tset = listnode_head (cmap->transform_set);
    if (tset == NULL)
      goto ERR;

    if (afi == AFI_IP)
      {                     
        /* IPSec SA section. */
        zsnprintf (buf, sizeof (buf), "%d", cmap->accesslist_id);

        al = access_list_lookup (vr, AFI_IP, buf);
        if (al != NULL)
          {
            mfilter = al->head;
            while (mfilter)
             {
               /* Only permits are IPSec'ed. */ 
               if (mfilter->type != FILTER_PERMIT)
                 continue; 
                   
               filter = &mfilter->u.cfilter;
               if (filter->extended)
                 {
                   /* Open section. */
                   fputs (IPSEC_TAG, fp);

                   /* Write flow addresses. */
                   pal_mem_set (&q, 0, sizeof (q));
                   q.family = AF_INET;
                   addr.s_addr = ~filter->addr_mask.s_addr;
                   q.prefixlen = ip_masklen (addr);
                   q.u.prefix4 = filter->addr;
                   prefix2str (&q, buf, sizeof (buf));\

                   fputs (IP_ADDRESS_TAG, fp);
                   fputs (buf, fp);
                   fputs (SPACE_TAG, fp);
                   fputs (IP_ADDRESS_TAG, fp);
                   pal_mem_set (&q, 0, sizeof (q));
                   q.family = AF_INET;
                   addr.s_addr = ~filter->mask_mask.s_addr;
                   q.prefixlen = ip_masklen (addr);
                   q.u.prefix4 = filter->mask;
                   prefix2str (&q, buf, sizeof (buf));
                   fputs (buf, fp);
 
                   fputs (OPEN_TAG, fp);
                   fputs (NEWLINE, fp);

                   /* IKE id. */
                   PUT_IKEID(buf, num, fp);
                   /* Lifetime Section */
                   /* Use the lifetime configured for the crypto-map */
                   if ((cmap->sec_lifetime != 0) && 
                       (ipsec_master->sec_association_lifetime != 
                                                         cmap->sec_lifetime))
                     {
                       zsnprintf (buf, sizeof (buf), "%s %d %s %s", LIFETIME, 
                                  cmap->sec_lifetime, SECS, TERMINATOR_TAG);
                       fputs (buf, fp);
                       fputs (NEWLINE, fp);
                     }
                   else if ((cmap->byte_lifetime != 0) &&
                            (ipsec_master->byte_association_lifetime !=
                                                         cmap->byte_lifetime))
                     {
                       zsnprintf (buf, sizeof (buf), "%s %d %s %s", LIFETIME,
                                  cmap->byte_lifetime, KBYTES, TERMINATOR_TAG);
                       fputs (buf, fp);
                       fputs (NEWLINE, fp);
                     }
                   else if (ipsec_master->sec_association_lifetime !=
                                                IPSEC_SA_SEC_LIFETIME_DEFAULT)
                     {
                       /* use the global configured lifetime when lifetime 
                          for crypto-map is not configured 
                       */
                       zsnprintf (buf, sizeof (buf), "%s %d %s %s", LIFETIME,
                                  ipsec_master->sec_association_lifetime,
                                  SECS, TERMINATOR_TAG);
                       fputs (buf, fp);
                       fputs (NEWLINE, fp);
                     }
                   else if (ipsec_master->sec_association_lifetime !=
                                                IPSEC_SA_BYTE_LIFETIME_DEFAULT)
                     {
                       zsnprintf (buf, sizeof (buf), "%s %d %s %s", LIFETIME,
                                  ipsec_master->byte_association_lifetime,
                                  KBYTES, TERMINATOR_TAG);
                       fputs (buf, fp);
                       fputs (NEWLINE, fp);
                     }
                   
                   /* Proposal section. */
                   fputs (PROPOSAL_TAG, fp);

                   /* Open tag. */
                   fputs (OPEN_TAG, fp);
                   fputs (NEWLINE, fp);

                   /* Group. */
                   PUT_DHGROUP(buf, isakmp, fp);

                   /* Put Encryption algorithm. */
                   PUT_ENC_ALGO(first,atleastone,cmap,tset,node3,tstr);

                   /* Put Integrity algorithm. */
                   PUT_INTEGRITY_ALGO(first,atleastone,cmap,tset,node3,tstr);

                   fputs (CLOSE_TAG, fp);
                   fputs (NEWLINE, fp);
                   fputs (CLOSE_TAG, fp);
                   fputs (NEWLINE, fp);

                 }
                   
               mfilter = mfilter->next;
             }
          }
        else
          goto ERR;
      }
#ifdef HAVE_IPV6
    else
      {
        /* Open section. */
        fputs (IPSEC_TAG, fp);

        /* Write flow addresses. 
           TBD only for all flows for now. */

        al = access_list_lookup (vr, AFI_IP6, cmap->ipv6_acl_name);
        if (al != NULL)
          {
            struct filter_pacos_ext *z_ext_filter;

            mfilter = al->head;
            z_ext_filter = &mfilter->u.zextfilter;
            pal_mem_set (&q, 0, sizeof (q));
            q.family = AF_INET6;
            q.u.prefix6 = z_ext_filter->sprefix.u.prefix6;
            q.prefixlen = z_ext_filter->sprefix.prefixlen;
            prefix2str (&q, buf, sizeof (buf));

            fputs (IP_ADDRESS_TAG, fp);
            fputs (buf, fp);
            fputs (SPACE_TAG, fp);
            fputs (IP_ADDRESS_TAG, fp);

            pal_mem_set (&q, 0, sizeof (q));
            q.family = AF_INET6;
            q.u.prefix6 = z_ext_filter->dprefix.u.prefix6;
            q.prefixlen = z_ext_filter->dprefix.prefixlen;
            prefix2str (&q, buf, sizeof (buf));
            fputs (buf, fp);

            fputs (OPEN_TAG, fp);
            fputs (NEWLINE, fp);

            /* IKE id. */
            PUT_IKEID(buf, num, fp);

            /* Proposal section. */
            fputs (PROPOSAL_TAG, fp);

            /* Open tag. */
            fputs (OPEN_TAG, fp);
            fputs (NEWLINE, fp);

            /* Group. */
            PUT_DHGROUP(buf, isakmp, fp);

           /* Put Encryption algorithm. */
           PUT_ENC_ALGO(first,atleastone,cmap,tset,node3,tstr);

           /* Put Integrity algorithm. */
           PUT_INTEGRITY_ALGO(first,atleastone,cmap,tset,node3,tstr);

           fputs (CLOSE_TAG, fp);
           fputs (NEWLINE, fp);
           fputs (CLOSE_TAG, fp);
           fputs (NEWLINE, fp);
         }
       else
         goto ERR;

      } 
#endif /* HAVE_IPV6 */
  }
  

  if (fp)
    fclose (fp);

  /* SIGHUP IKE. */
  _pal_reconfig_ike ();
  list_delete (tlist);

  return 0;

ERR:
  if (fp)
    fclose (fp);

  return -1;
}
#endif /*HAVE_IPNET */
