/* Copyright (C) 2005  All Rights Reserved. */
#ifndef _PAL_IPSEC_PFKEYV2_H
#define __PAL_IPSEC_PFKEYV2_H

#ifdef HAVE_IPNET

/*
 ******************************************************************************
 *                     INTERPEAK API HEADER FILE
 *
 *   Document no: @(#) PacOS-7-8-1 pal_ipsec_pfkeyv2.h,v 1.2
 *   /cvsroot/pacos/PacOS5/pal/linux/pal_ipsec_pfkeyv2.h,v
 *   bob.macgill 2008/01/30 20:29:30
 *   Exp 
 *
 *   Copyright 2000-2005 Interpeak AB (http://www.interpeak.se). All rights reserved.
 *   Design and implementation by Lennart Bang <lob@interpeak.se>
 ******************************************************************************
 */
#ifndef PFKEY_V2_H
#define PFKEY_V2_H 1

/*
 ****************************************************************************
 * 1                    DESCRIPTION
 ****************************************************************************
 * This file defines structures and symbols for the PF_KEY Version 2
 * key management interface. It was written at the U.S. Naval Research
 * Laboratory. This file is in the public domain. The authors ask that
 * you leave this credit intact on any copies of this file.
 */

/*
 ****************************************************************************
 * 2                    CONFIGURATION
 ****************************************************************************
 */


/*
 ****************************************************************************
 * 3                    REVISION
 ***************************************************************************
 */
#define PF_KEY_V2            2
#define PFKEYV2_REVISION     199806L


/*
 ****************************************************************************
 * 4                    INCLUDE FILES
 ****************************************************************************
 */

/* #include <ipcom_type.h>

#endif */


/*
 ****************************************************************************
 * 5                    DEFINES
 ****************************************************************************
 */

#define SADB_SATYPE_UNSPEC           0
#define SADB_SATYPE_AH               2
#define SADB_SATYPE_ESP              3
#define SADB_SATYPE_RSVP             5
#define SADB_SATYPE_OSPFV2           6
#define SADB_SATYPE_RIPV2            7
#define SADB_SATYPE_MIP              8
#define SADB_X_SATYPE_APNP           9
#define SADB_X_SATYPE_TCPSIGNATURE  10
#define SADB_X_SATYPE_BYPASS        11
#define SADB_SATYPE_MAX             11


/*
 *===========================================================================
 *                         SADB_EXT_
 *===========================================================================
 */
#define SADB_EXT_RESERVED             0    /* _Extension_data_type_ */
#define SADB_EXT_SA                   1    /* struct sadb_sa */
#define SADB_EXT_LIFETIME_CURRENT     2    /* struct sadb_lifetime */
#define SADB_EXT_LIFETIME_HARD        3    /* struct sadb_lifetime */
#define SADB_EXT_LIFETIME_SOFT        4    /* struct sadb_lifetime */
#define SADB_EXT_ADDRESS_SRC          5    /* struct sadb_address */
#define SADB_EXT_ADDRESS_DST          6    /* struct sadb_address */
#define SADB_EXT_ADDRESS_PROXY        7    /* struct sadb_address */
#define SADB_EXT_KEY_AUTH             8    /* struct sadb_key */
#define SADB_EXT_KEY_ENCRYPT          9    /* struct sadb_key */
#define SADB_EXT_IDENTITY_SRC         10   /* struct sadb_ident */
#define SADB_EXT_IDENTITY_DST         11   /* struct sadb_ident */
#define SADB_EXT_SENSITIVITY          12   /* struct sadb_sens */
#define SADB_EXT_PROPOSAL             13   /* struct sadb_prop */
#define SADB_EXT_SUPPORTED            14   /* struct sadb_supported */
#define SADB_EXT_SPIRANGE             15   /* struct sadb_spirange */
#define SADB_X_EXT_SRC_MASK           16   /* struct sadb_address */
#define SADB_X_EXT_DST_MASK           17   /* struct sadb_address */
#define SADB_X_EXT_PROTOCOL           18   /* struct sadb_protocol */
#define SADB_X_EXT_SA2                19   /* struct sadb_sa */
#define SADB_X_EXT_SRC_FLOW           20   /* struct sadb_address */
#define SADB_X_EXT_DST_FLOW           21   /* struct sadb_address */
#define SADB_X_EXT_DST2               22   /* struct sadb_address */
#define SADB_X_EXT_FLOW_TYPE          23   /* struct sadb_protocol */
#define SADB_X_EXT_UDPENCAP           24   /* struct sadb_x_udpencap */
#define SADB_EXT_MAX                  24


/* sadb_sa_flags */
#define SADB_SAFLAGS_PFS                0x001L    /* perfect forward secrecy */
#define SADB_X_SAFLAGS_HALFIV           0x002L    /* Used for ESP-old */
#define SADB_X_SAFLAGS_TUNNEL           0x004L    /* Force tunneling */
#define SADB_X_SAFLAGS_CHAINDEL         0x008L    /* Delete whole SA chain */
#define SADB_X_SAFLAGS_REPLACEFLOW      0x020L    /* Replace existing flow */
#define SADB_X_SAFLAGS_INGRESS_FLOW     0x040L    /* Ingress ACL entry */
#define SADB_X_SAFLAGS_RANDOMPADDING    0x080L    /* Random ESP padding */
#define SADB_X_SAFLAGS_NOREPLAY         0x100L    /* No replay counter */
#define SADB_X_SAFLAGS_UDPENCAP         0x200L    /* ESP in UDP */

  /* new sadb_sa_flags by Interpeak: */
#define SADB_X_SAFLAGS_INGRESS_SA       0x040L    /* Ingress SA entry (same val as INGRESS_FLOW) */
#define SADB_X_SAFLAGS_EGRESS_SA        0x400L    /* Egress (output) SA entry */


/*
 ****************************************************************************
 * 5                    TYPES
 ****************************************************************************
 */

/*
 *===========================================================================
 *                         sadb_msg
 *===========================================================================
 */
struct sadb_msg
{
    u_int8_t    sadb_msg_version;
    u_int8_t    sadb_msg_type;
    u_int8_t    sadb_msg_errno;
    u_int8_t    sadb_msg_satype;
    u_int16_t   sadb_msg_len;
    u_int16_t   sadb_msg_reserved;
    u_int32_t   sadb_msg_seq;
    u_int32_t   sadb_msg_pid;
}; 

/* sadb_msg_type */
#define SADB_RESERVED    0
#define SADB_GETSPI      1
#define SADB_UPDATE      2
#define SADB_ADD         3
#define SADB_DELETE      4
#define SADB_GET         5
#define SADB_ACQUIRE     6
#define SADB_REGISTER    7
#define SADB_EXPIRE      8
#define SADB_FLUSH       9
#define SADB_DUMP        10
#define SADB_X_PROMISC   11
#define SADB_X_ADDFLOW   12
#define SADB_X_DELFLOW   13
#define SADB_X_GRPSPIS   14
#define SADB_X_BINDSA    15
#define SADB_MAX         15


/*
 *===========================================================================
 *                         sadb_ext
 *===========================================================================
 */
struct sadb_ext
{
    u_int16_t   sadb_ext_len;
    u_int16_t   sadb_ext_type;
};


/*
 *===========================================================================
 *                         sadb_sa
 *===========================================================================
 */
struct sadb_sa
{
    u_int16_t   sadb_sa_len;
    u_int16_t   sadb_sa_exttype;
    u_int32_t   sadb_sa_spi;
    u_int8_t    sadb_sa_replay;
    u_int8_t    sadb_sa_state;
    u_int8_t    sadb_sa_auth;
    u_int8_t    sadb_sa_encrypt;
    u_int32_t   sadb_sa_flags;
};


/* sadb_sa_state */
#define SADB_SASTATE_LARVAL   0
#define SADB_SASTATE_MATURE   1
#define SADB_SASTATE_DYING    2
#define SADB_SASTATE_DEAD     3
#define SADB_SASTATE_MAX      3

#define SADB_KEY_FLAGS_MAX    0

/* sadb_sa_auth */
#define SADB_AALG_NONE               0
#define SADB_AALG_MD5HMAC            1
#define SADB_AALG_SHA1HMAC           2
#define SADB_AALG_MD5HMAC96          3
#define SADB_AALG_SHA1HMAC96         4
#define SADB_X_AALG_RIPEMD160HMAC96  5
#define SADB_X_AALG_MD5              6
#define SADB_X_AALG_SHA1             7
#define SADB_X_AALG_AESXCBCMAC96     8
#define SADB_AALG_MAX                8

/* sadb_sa_encrypt */
#define SADB_EALG_NONE               0
#define SADB_EALG_DESCBC             1
#define SADB_EALG_3DESCBC            2
#define SADB_X_EALG_BLF              3
#define SADB_X_EALG_CAST             4
#define SADB_X_EALG_SKIPJACK         5    /* not supported */
#define SADB_X_EALG_AES              6
#define SADB_EALG_MAX                6

/*
 *===========================================================================
 *                         sadb_lifetime
 *===========================================================================
 */
struct sadb_lifetime
{
    u_int16_t   sadb_lifetime_len;
    u_int16_t   sadb_lifetime_exttype;
    u_int32_t   sadb_lifetime_allocations;
    u_int64_t   sadb_lifetime_bytes;
    u_int64_t   sadb_lifetime_addtime;
    u_int64_t   sadb_lifetime_usetime;
};



/*
 *===========================================================================
 *                         sadb_address
 *===========================================================================
 */
struct sadb_address
{
    u_int16_t   sadb_address_len;
    u_int16_t   sadb_address_exttype;
    u_int8_t    sadb_address_proto;
    u_int8_t    sadb_address_prefixlen;
    u_int16_t   sadb_address_reserved;
};


/*
 *===========================================================================
 *                         sadb_key
 *===========================================================================
 */
struct sadb_key
{
    u_int16_t   sadb_key_len;
    u_int16_t   sadb_key_exttype;
    u_int16_t   sadb_key_bits;
    u_int16_t   sadb_key_reserved;
};


/*
 *===========================================================================
 *                         sadb_ident
 *===========================================================================
 */
struct sadb_ident
{
    u_int16_t   sadb_ident_len;
    u_int16_t   sadb_ident_exttype;
    u_int16_t   sadb_ident_type;
    u_int16_t   sadb_ident_reserved;
    u_int64_t   sadb_ident_id;
};


/* sadb_ident_type */
#define SADB_IDENTTYPE_RESERVED   0
#define SADB_IDENTTYPE_PREFIX     1
#define SADB_IDENTTYPE_FQDN       2
#define SADB_IDENTTYPE_USERFQDN   3
#define SADB_IDENTTYPE_MAX        3


/*
 *===========================================================================
 *                         sadb_sens
 *===========================================================================
 */
struct sadb_sens
{
    u_int16_t   sadb_sens_len;
    u_int16_t   sadb_sens_exttype;
    u_int32_t   sadb_sens_dpd;
    u_int8_t    sadb_sens_sens_level;
    u_int8_t    sadb_sens_sens_len;
    u_int8_t    sadb_sens_integ_level;
    u_int8_t    sadb_sens_integ_len;
    u_int32_t   sadb_sens_reserved;
};


/*
 *===========================================================================
 *                         sadb_prop
 *===========================================================================
 */
struct sadb_prop
{
    u_int16_t   sadb_prop_len;
    u_int16_t   sadb_prop_exttype;
    u_int8_t    sadb_prop_replay;
    u_int8_t    sadb_prop_reserved[3];
};


/*
 *===========================================================================
 *                         sadb_comb
 *===========================================================================
 */
struct sadb_comb
{
    u_int8_t    sadb_comb_auth;
    u_int8_t    sadb_comb_encrypt;
    u_int16_t   sadb_comb_flags;
    u_int16_t   sadb_comb_auth_minbits;
    u_int16_t   sadb_comb_auth_maxbits;
    u_int16_t   sadb_comb_encrypt_minbits;
    u_int16_t   sadb_comb_encrypt_maxbits;
    u_int32_t   sadb_comb_reserved;
    u_int32_t   sadb_comb_soft_allocations;
    u_int32_t   sadb_comb_hard_allocations;
    u_int64_t   sadb_comb_soft_bytes;
    u_int64_t   sadb_comb_hard_bytes;
    u_int64_t   sadb_comb_soft_addtime;
    u_int64_t   sadb_comb_hard_addtime;
    u_int64_t   sadb_comb_soft_usetime;
    u_int64_t   sadb_comb_hard_usetime;
};


/*
 *===========================================================================
 *                         sadb_suppported
 *===========================================================================
 * original sadb_supported struct:
 *    struct sadb_supported
 *    {
 *       uint16_t   sadb_supported_len;
 *       uint16_t   sadb_supported_exttype;
 *       uint32_t   sadb_supported_reserved;
 *    };
 */
struct sadb_supported
{
    u_int16_t  sadb_supported_len;
    u_int16_t  sadb_supported_exttype;
    u_int8_t   sadb_supported_nauth;
    u_int8_t   sadb_supported_nencrypt;
    u_int16_t  sadb_supported_reserved;
};



/*
 *===========================================================================
 *                         sadb_alg
 *===========================================================================
 */
struct sadb_alg
{
    u_int8_t    sadb_alg_id;
    u_int8_t    sadb_alg_ivlen;
    u_int16_t   sadb_alg_minbits;
    u_int16_t   sadb_alg_maxbits;
    u_int16_t   sadb_alg_reserved;
};


/*
 *===========================================================================
 *                         sadb_spirange
 *===========================================================================
 */
struct sadb_spirange
{
    u_int16_t   sadb_spirange_len;
    u_int16_t   sadb_spirange_exttype;
    u_int32_t   sadb_spirange_min;
    u_int32_t   sadb_spirange_max;
    u_int32_t   sadb_spirange_reserved;
};


/*
 *===========================================================================
 *                         sadb_protocol
 *===========================================================================
 */

struct sadb_protocol
{
    u_int16_t   sadb_protocol_len;
    u_int16_t   sadb_protocol_exttype;
    u_int8_t    sadb_protocol_proto;
    u_int8_t    sadb_protocol_reserved1;
    u_int16_t   sadb_protocol_reserved2;
};

/* SADB_X_EXT_FLOW_TYPE sadb_protocol_proto */
#define SADB_X_FLOW_TYPE_USE           1   /* Apply security (standard flow) */
#define SADB_X_FLOW_TYPE_ACQUIRE       2   /* Generate acquire message, send packet in cleartext */
#define SADB_X_FLOW_TYPE_REQUIRE       3   /* Generate acquire message, discard packet */
#define SADB_X_FLOW_TYPE_BYPASS        4   /* Send without applying security, i.e. cleartext */
#define SADB_X_FLOW_TYPE_DENY          5   /* Silent discard */
/*#define SADB_X_FLOW_TYPE_DONTACQ       6*/


/*
 *===========================================================================
 *                         sadb_x_udpencap
 *===========================================================================
 */

struct sadb_x_udpencap
{
    u_int16_t sadb_x_udpencap_len;
    u_int16_t sadb_x_udpencap_exttype;
    u_int16_t sadb_x_udpencap_port;
    u_int16_t sadb_x_udpencap_reserved;
};


/*
 ****************************************************************************
 *                      END OF FILE
 ****************************************************************************
 */

/* __PFKEY_V2_H */

#endif
#endif
#endif
