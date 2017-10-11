/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _PACOS_MPLS_H
#define _PACOS_MPLS_H

#include "lib/mpls/label_pool.h"
#include "prefix.h"

#define VPLS_ID_MIN            1
#define VPLS_ID_MAX            4294967295UL
#define VPLS_MAC_ADDR_LEN      6

/* LSP TYPE, bypass treated as primary */
#define LSP_TYPE_PRIMARY           1
#define LSP_TYPE_SECONDARY         2
#define LSP_TYPE_BACKUP            3
#define LSP_TYPE_BYPASS            4

#define MPLS_VC_STYLE_MARTINI              1
#define MPLS_VC_STYLE_VPLS_MESH            2
#define MPLS_VC_STYLE_VPLS_SPOKE           3

/* For unnumbered address */
#define AFI_UNNUMBERED                     255
#define AF_UNNUMBERED                      255
#define IPV4_BROADCAST_ADDR                0xFFFFFFFF

#ifdef WORDS_BIGENDIAN
#define MPLS_BITFIELDS2(A,B)       A; B
#define MPLS_BITFIELDS3(A,B,C)     A; B; C
#define MPLS_BITFIELDS4(A,B,C,D)   A; B; C; D
#else /* ! WORDS_BIGENDIAN  */
#define MPLS_BITFIELDS2(A,B)       B; A
#define MPLS_BITFIELDS3(A,B,C)     C; B; A
#define MPLS_BITFIELDS4(A,B,C,D)   D; C; B; A
#endif /* WORDS_BIGENDIAN  */

#define BIDIR_LSP_NAME_LENGTH         10

typedef enum mpls_row_status
  {
    RS_ACTIVE = 1,
    RS_NOT_IN_SERVICE,
    RS_NOT_READY,
    RS_CREATE_GO,
    RS_CREATE_WAIT,
    RS_DESTROY
  } mpls_row_status_t;

typedef enum mpls_admn_status
  {
    ADMN_UP = 1,
    ADMN_DOWN,
    ADMN_TESTING
  } mpls_admn_status_t;

#include "prefix.h"

/* As per the MPLS-TC-STD mib */
typedef enum mpls_owner_type
  {
    MPLS_UNKNOWN = 1,
    MPLS_OTHER,
    MPLS_SNMP,
    MPLS_LDP,
    MPLS_CRLDP,
    MPLS_RSVP,
    MPLS_POLICYAGENT,
    MPLS_OTHER_BGP,
    MPLS_OTHER_CLI,
    MPLS_OTHER_LDP_VC,
    MPLS_IGP_SHORTCUT,
  } mpls_owner_t;

typedef enum mpls_inet_address_type
  {
    INET_AD_UNKNOWN = 0,
    INET_AD_IPV4,
    INET_AD_IPV6,
    INET_AD_IPV4Z,
    INET_AD_IPV6Z,
    INET_AD_DNS
  } mpls_inet_address_type_t;

enum gmpls_entry_type
{
  gmpls_entry_type_error = -1,
  gmpls_entry_type_ip = 1,
  gmpls_entry_type_pbb_te,
  gmpls_entry_type_tdm,
  gmpls_entry_type_max = gmpls_entry_type_tdm
};

/* Key used by RSVP-TE protocol for IPV4 */
struct rsvp_key_ipv4
{
  u_int16_t trunk_id;
  u_int16_t lsp_id;
  struct pal_in4_addr ingr;
  struct pal_in4_addr egr;
};

/* Key used by RSVP-TE protocol for IPV6 */
#ifdef HAVE_IPV6
struct rsvp_key_ipv6
{
  u_int16_t trunk_id;
  u_int16_t lsp_id;
  struct pal_in6_addr ingr;
  struct pal_in6_addr egr;
};
#endif /* HAVE_IPV6 */

/* Generic RSVP_TE Key */
struct rsvp_key
{
  afi_t afi;
  u_int16_t len;
  union
  {
    u_char key;
    struct rsvp_key_ipv4 ipv4;
#ifdef HAVE_IPV6
    struct rsvp_key_ipv6 ipv6;
#endif /* HAVE_IPV6 */
  } u;
};

/* Key used by LDP protocol using for IPV4*/
struct ldp_key_ipv4
{
  struct pal_in4_addr addr;
  u_int32_t lsr_id;
  u_int16_t label_space;
};

/* Key used by LDP protocol using for IPV6*/
#ifdef HAVE_IPV6
struct ldp_key_ipv6
{
  struct pal_in6_addr addr;
  u_int32_t lsr_id;
  u_int16_t label_space;
};
#endif /* HAVE_IPv6 */

/* Generic ldp key */
struct ldp_key
{
  afi_t afi;
  union
  {
    struct ldp_key_ipv4 ipv4;
#ifdef HAVE_IPV6
    struct ldp_key_ipv6 ipv6;
#endif /* HAVE_IPv6 */
  } u;
};

/* LDP VC key. */
struct ldp_vc_key
{
  u_int32_t vc_id;
  struct pal_in4_addr vc_peer;
};

/* Key used by BGP protocol for IPv4. */
struct bgp_key_ipv4
{
  struct pal_in4_addr peer;
  struct prefix_ipv4 p;
};

/* Key used by BGP protocol for IPV6. */
#ifdef HAVE_IPV6
struct bgp_key_ipv6
{
  struct pal_in6_addr peer;
  struct prefix_ipv6 p;
};
#endif /* HAVE_IPv6 */

/* Generic bgp key */
struct bgp_key
{
  afi_t afi;
  union
  {
    struct bgp_key_ipv4 ipv4;
#ifdef HAVE_IPV6
    struct bgp_key_ipv6 ipv6;
#endif /* HAVE_IPV6 */
  } u;
  u_int32_t vrf_id;
};

#include "prefix.h"

struct crldp_key
{
  int dummy;
};

#define MPLS_OWNER_KEY_VAL_LEN \
  MAX ((MAX (sizeof (struct rsvp_key), sizeof (struct ldp_key))), \
       (MAX (sizeof (struct bgp_key), sizeof (struct crldp_key))))

struct mpls_owner
{
  u_char owner;
  union
  {
    struct rsvp_key r_key;
    struct ldp_key l_key;
    struct bgp_key b_key;
    struct crldp_key c_key;
    struct ldp_vc_key vc_key;
    u_char val[MPLS_OWNER_KEY_VAL_LEN];
  } u;
};


typedef enum mpls_notification_type
  {
    MPLS_NTF_ILM_ADD_FAILURE = 0,
    MPLS_NTF_FTN_ADD_FAILURE,
    MPLS_NTF_VC_FTN_ADD_FAILURE,
    MPLS_NTF_VC_ILM_ADD_FAILURE
  } mpls_notification_t;


typedef enum mpls_status_code
  {
    MPLS_CODE_INVALID_LABEL = 0
  } mpls_status_code_t;

#ifdef HAVE_IPV6
#define ADDR_IN_LEN     sizeof (struct pal_in6_addr)
#else
#define ADDR_IN_LEN     sizeof (struct pal_in4_addr)
#endif /* HAVE_IPV6 */

/* IP address structure */
struct addr_in
{
  afi_t afi;
  union
  {
    struct pal_in4_addr ipv4;
#ifdef HAVE_IPV6
    struct pal_in6_addr ipv6;
#endif /* HAVE_IPV6 */
    u_char val[ADDR_IN_LEN];
  } u;
};

/*
   LSP ID value used in XC entries
   a) 2 byte value for RSVP-TE
   b) 6 bytes value for CR-LDP
   c) Not used for LDP entries
*/
struct mpls_lspid
{
  union
  {
    u_char cr_lspid[6];
    u_int16_t rsvp_lspid;
  }u;
};

typedef enum ftn_action_type
  {
    DROP = 1,
    REDIRECT_LSP,
    REDIRECT_TUNNEL
  } ftn_action_type_t;


typedef enum lsp_type
  {
    LSP_DEFAULT = 0,
    ELSP_CONFIG,
    ELSP_SIGNAL,
    LLSP
  } lsp_type_t;

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
struct fec_entry_pbb_te
{
  /* PBB TE ISID */
  u_char isid [3];
};

struct pbb_te_label
{
  u_int16_t   bvid;
  u_char bmac [6];
};

#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */

struct gmpls_gen_label
{
  enum gmpls_entry_type type;

  union {
   u_int32_t pkt;
#ifdef HAVE_GMPLS
#ifdef HAVE_TDM
   struct tdm_label tdm;
#endif /* HAVE_TDM */
#ifdef HAVE_PBB_TE
   struct pbb_te_label pbb;
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  } u;
};

#ifdef HAVE_DIFFSERV

struct ds_info
{
  lsp_type_t lsp_type;

  /* DSCP-to-EXP mapping for ELSP. */
  u_char dscp_exp_map[8];

  /* DSCP value for LLSP. */
  u_char dscp;

  /* AF set for LLSP. */
  u_char af_set;
};

#endif /* HAVE_DIFFSERV */

#define IGP_SHORTCUT_METRIC_MIN                 1
#define IGP_SHORTCUT_METRIC_DEFAULT            65
struct lsp_bits
{
  MPLS_BITFIELDS4 (u_int32_t lsp_type :8,
                   u_int32_t act_type :4,
                   u_int32_t igp_shortcut :4,
                   /* igp-shortcut lsp_metric */
                   u_int32_t lsp_metric :16);

};


#define IGP_SHORTCUT_ENABLED   1
#define IGP_SHORTCUT_DISABLED  0

struct ftn_add_data
{
  /* Flags to specify data type.  */
  u_int16_t flags;
#define NSM_MSG_FTN_ADD           (1 << 0)
#define NSM_MSG_FTN_FAST_DELETE   (1 << 1)
#define NSM_MSG_FTN_DSCP_IN       (1 << 2)
#define NSM_MSG_FTN_GMPLS         (1 << 3)
#define NSM_MSG_FTN_BIDIR         (1 << 4)
#define NSM_MSG_FTN_INACT         (1 << 5)

  u_int32_t id;

  /* Owner information.  */
  struct mpls_owner owner;

  /* VRF ID; 0 for global ftn table */
  u_int32_t vrf_id;

  /* FEC for the FTN entry */
  struct prefix fec_prefix;

  /* Unique FTN index for an entry */
  u_int32_t ftn_ix;

#ifdef HAVE_DIFFSERV
  struct ds_info ds_info;
#endif /* HAVE_DIFFSERV */

#ifdef HAVE_GMPLS
  u_int32_t rev_idx;
#endif /* HAVE_GMPLS */
  struct addr_in nh_addr;
  u_int32_t out_label;
  u_int32_t oif_ix;
  struct mpls_lspid lspid;
  u_int32_t exp_bits;
  u_char dscp_in;

  union
  {
    struct lsp_bits bits;
    u_int32_t data;
  } lsp_bits;

  u_char opcode;
  u_int32_t tunnel_id;
  u_int32_t protected_lsp_id;
  u_int32_t qos_resrc_id;
  u_int32_t bypass_ftn_ix;

  char *sz_desc;

  /* Optional. For rsvp mapped route only. */
  struct prefix *pri_fec_prefix;
};

struct ftn_del_data
{
  u_int32_t vrf_id;
  struct prefix fec;
  u_int32_t ftn_ix;
};

struct ftn_ret_data
{
  /* Flags to specify data type.  */
  u_int16_t flags;

  /* FTN message ID.  */
  u_int32_t id;

  struct mpls_owner owner;
  u_int32_t ftn_ix;
  u_int32_t xc_ix;
  u_int32_t nhlfe_ix;
};

#ifdef HAVE_BFD
/**@brief Sturcture holding the NSM_MSG_MPLS_FTN_DOWN message sent by NSM
 * to the LSP Owner (LDP/RSVP).
 */
struct ftn_down_data
{
  u_char ftn_owner;
  union
  {
    struct rsvp_key_ipv4 rsvp_key;
    struct ldp_key_ipv4 ldp_key;
  }u;
};
#endif /* HAVE_BFD */

#define NSM_MSG_ILM_ADD                     (1 << 0)
#define NSM_MSG_ILM_FAST_DELETE             (1 << 1)
#define NSM_MSG_ILM_DELETE                  (1 << 2)
#define NSM_MSG_ILM_BYPASS                  (1 << 3)
#define NSM_MSG_ILM_BIDIR                   (1 << 4)
#define NSM_MSG_ILM_FAST_ADD                (1 << 5)

#ifdef HAVE_RESTART
#define NSM_MSG_ILM_OUT_LABEL_NOT_REFRESHED (1 << 6)
#define NSM_MSG_ILM_STALE_DB_AVAILABLE      (1 << 7)
#endif /* HAVE_RESTART */
/* The below flag is used when the ILM data is converted from old format to
   new. This means that the gmpls type return data should not be returned.
   Instead MPLS type return data needs to be returned */
#define NSM_MSG_ILM_GEN_MPLS                (1 << 8)
#define NSM_MSG_GEN_ILM_FWD                 (1 << 9)
#define NSM_MSG_GEN_ILM_INACTIVE            (1 << 10)
#define NSM_MSG_ILM_TRANS_MPLS              (1 << 11)

struct ilm_add_data
{
  /* For NSM server.  */
  u_int16_t flags;

  u_int32_t id;

  /* Owner information.  */
  struct mpls_owner owner;

  /* Incoming Interface Index */
  u_int32_t iif_ix;

  /* Incoming Label */
  u_int32_t in_label;

  /* Unique ILM index */
  u_int32_t ilm_ix;

#ifdef HAVE_DIFFSERV
  struct ds_info ds_info;
#endif /* HAVE_DIFFSERV */

  u_int32_t oif_ix;
  u_int32_t out_label;
  struct addr_in nh_addr;
  struct mpls_lspid lspid;
  u_int32_t n_pops;
  struct prefix fec_prefix;
  u_int32_t qos_resrc_id;
  u_char opcode;
  u_char primary;
};

struct ilm_del_data
{
  u_int32_t iif_ix;
  u_int32_t in_label;
  u_int32_t ilm_ix;
  u_int16_t flags;
};

/* ILM & FTN Ret data flags */
#define NSM_MSG_REPLY_TO_FTN_ADD        (1 << 0)
#define NSM_MSG_REPLY_TO_ILM_ADD        (1 << 0)
#define NSM_MSG_REPLY_TO_BIDIR          (1 << 1)
#define NSM_MSG_REPLY_TO_FWD            (1 << 2)
#define NSM_MSG_REPLY_TO_GEN_ILM_ADD    (1 << 3)

struct ilm_gen_ret_data
{
  /* Flags to specify data type.  */
  u_int16_t flags;

  /* For NSM server.  */
  u_int32_t id;

  struct mpls_owner owner;
  u_int32_t iif_ix;
  struct gmpls_gen_label in_label;
  u_int32_t ilm_ix;
  u_int32_t xc_ix;
  u_int32_t nhlfe_ix;
};

struct ilm_ret_data
{
  /* Flags to specify data type.  */
  u_int16_t flags;

  /* For NSM server.  */
  u_int32_t id;

  struct mpls_owner owner;
  u_int32_t iif_ix;
  u_int32_t in_label;
  u_int32_t ilm_ix;
  u_int32_t xc_ix;
  u_int32_t nhlfe_ix;
};

struct mpls_notification
{
  /* Id. */
  u_int32_t id;

  struct mpls_owner owner;
  u_char type;
  u_char status;
};

#ifdef HAVE_GMPLS

#ifdef HAVE_TDM
struct fec_entry_tdm
{
  /* physical interface identifier */
  u_int32_t gifndex;

  /* Time slot identifier */
  u_int32_t tslot;
};

struct tdm_label
{
  u_int32_t gifndex;
  u_int32_t tslot;
};

#endif /* HAVE_TDM */

#endif /* HAVE_GMPLS */

struct fec_gen_entry
{
  enum gmpls_entry_type type;

  /* Union of all FTN types */
  union {
    struct prefix prefix;
#ifdef HAVE_GMPLS
#ifdef HAVE_TDM
    struct fec_entry_tdm tdm;
#endif /* HAVE_TDM */
#ifdef HAVE_PBB_TE
    struct fec_entry_pbb_te pbb;
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  } u;
};

/* Added for GELS : to introduce the technology specific data in the
   ILM and FTN structures */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE

struct pbb_te_data
{
  u_int32_t tesid;
};
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */

/* Technology specific generic data in the ILM and FTN structures */
struct gmpls_tgen_data
{
  enum gmpls_entry_type gmpls_type;

  /* Union of all tech specific data types */
  union {
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
    struct pbb_te_data pbb;
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  } u;
};

 struct sonet_label_type
 {
   void* info;
 };

 struct sdh_label_type
 {
   void* info;
 };

 struct waveband_label_type
 {
   u_int32_t   waveband_id;
   u_int32_t   waveband_start_lbl;
   u_int32_t   waveband_end_lbl;

 };

#ifdef  HAVE_GMPLS
#ifdef  HAVE_PBB_TE

struct pbb_te_label_type
{
  u_int16_t   bvid;
  u_char bmac [6];
};
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */

 /**@brief A structure that provides the ability to reperesent any of the
    various GMPLS label forms.
   */
 struct generalized_label
 {
   /* Provides the label type being carried */
   enum gmpls_label_type gmpls_label_type;

   union label_types
   {
     u_int32_t                   mpls_label;
#ifdef  HAVE_GMPLS
#ifdef HAVE_PORTWAVELEGTH
     u_int32_t                   port_wavelength;
#endif
#ifdef HAVE_FREEFORM
     u_char                      freeform_label[64];
#endif
#ifdef HAVE_SONET
     struct sonet_label_type     sonet_label;
#endif
#ifdef HAVE_SDH
     struct sdh_label_type       sdh_label;
#endif
#ifdef HAVE_WAVEBAND
     struct waveband_label_type  waveband_label;
#endif
#ifdef  HAVE_PBB_TE
     struct pbb_te_label_type    pbb_label;
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
   } u;
 };

struct gmpls_label_data
{
  enum gmpls_entry_type type;

  /* label value detrmined based on the type above whose length is
   * calculated externally */
  void *label_value;
};

/* Flags for GEN FTN add data */
#define NSM_MSG_GEN_FTN_ADD          (1 << 0)
#define NSM_MSG_GEN_FTN_FAST_ADD     (1 << 1)
#define NSM_MSG_GEN_FTN_DEL          (1 << 2)
#define NSM_MSG_GEN_FTN_FAST_DEL     (1 << 3)
#define NSM_MSG_GEN_FTN_DSCP_IN      (1 << 4)
#define NSM_MSG_GEN_FTN_DUMMY        (1 << 5)
#define NSM_MSG_GEN_FTN_INACTIVE     (1 << 6)
#define NSM_MSG_GEN_FTN_BIDIR        (1 << 7)
#define NSM_MSG_GEN_FTN_FWD          (1 << 8)
#define NSM_MSG_GEN_FTN_MPLS         (1 << 9)
#define NSM_MSG_GEN_FTN_INACT        (1 << 10)

struct ftn_fast_add_gen_data
{
  u_int16_t flags;

  /* FTN message ID.  */
  u_int32_t id;

  /* Unique FTN index for an entry */
  u_int32_t ftn_ix;

  /* Fec type entry - value of FEC is not valid only is valid */
  struct fec_gen_entry fec;
};

struct ftn_add_gen_data
{
  /* Flags to specify data type.  */
  u_int16_t flags;

  /* FTN message ID.  */
  u_int32_t id;

  /* Owner information.  */
  struct mpls_owner owner;

  /* VRF ID; 0 for global ftn table */
  u_int32_t vrf_id;

  /* FEC for the FTN entry */
  struct fec_gen_entry ftn;

  /* Unique FTN index for an entry */
  u_int32_t ftn_ix;

#ifdef HAVE_GMPLS
  u_int32_t rev_ilm_ix;

  /* Reverse Cross connect Index if known for a bidirectional LSP */
  u_int32_t rev_xc_ix;
#endif /* HAVE_GMPLS */

#ifdef HAVE_PACKET
#ifdef HAVE_DIFFSERV
  struct ds_info ds_info;
#endif /* HAVE_DIFFSERV */
#endif /* HAVE_PACKET */

  struct addr_in nh_addr;
  struct gmpls_gen_label out_label;
  u_int32_t oif_ix;
  struct mpls_lspid lspid;
  u_int32_t exp_bits;
  u_char dscp_in;
  union
  {
    struct lsp_bits bits;
    u_int32_t data;
  } lsp_bits;

  u_char opcode;
  u_int32_t tunnel_id;
  u_int32_t protected_lsp_id;
  u_int32_t qos_resrc_id;
  u_int32_t bypass_ftn_ix;

  char *sz_desc;

  /* Optional. For rsvp mapped route only. */
  struct prefix *pri_fec_prefix;

/* GELS : For storing Tech specific data */
  struct gmpls_tgen_data *tgen_data;
};


/* The generalized API's can be used even when GMPLS is not used */
struct ftn_del_gen_data
{
  u_int32_t vrf_id;
  struct fec_gen_entry fec;
  u_int32_t ftn_ix;
  u_int16_t flags;
};

struct ilm_add_gen_data
{
  /* For NSM server.  */
  u_int16_t flags;

  u_int32_t id;

  /* Owner information.  */
  struct mpls_owner owner;

  /* Incoming Interface Index */
  u_int32_t iif_ix;

  /* Incoming Label */
  struct gmpls_gen_label in_label;

  /* FEC for the ILM entry */
  struct fec_gen_entry ilm;

  /* Unique ILM index */
  u_int32_t ilm_ix;

  /* Reverse */
#ifdef HAVE_GMPLS
  /* Either one of ftn_ix or ilm_ix is non-zero */
  u_int32_t rev_ftn_ix;
  u_int32_t rev_ilm_ix;

  /* Reverse Cross connect Index if known for a bidirectional LSP */
  u_int32_t rev_xc_ix;
#endif /* HAVE_GMPLS */

#ifdef HAVE_PACKET
#ifdef HAVE_DIFFSERV
  struct ds_info ds_info;
#endif /* HAVE_DIFFSERV */
#endif /* HAVE_PACKET */

  char *sz_desc;

  u_int32_t oif_ix;
  struct gmpls_gen_label out_label;
  struct addr_in nh_addr;
  struct mpls_lspid lspid;
  u_int32_t n_pops;
  struct prefix fec_prefix;
  u_int32_t qos_resrc_id;
  u_char opcode;
  u_char primary;

/* GELS : For storing Tech specific data */
  struct gmpls_tgen_data *tgen_data;
};

struct bidirectional_entry
{
  char b_name[BIDIR_LSP_NAME_LENGTH];

#define FTN_ENTRY_CFG            (1 << 0)
#define ILM_ENTRY_FORWARD        (1 << 1)
#define ILM_ENTRY_BACKWARD       (1 << 2)
#define BIDIR_LSP_STATUS         (1 << 3)
  u_char flags;

  /* Ftn entry */
  struct ftn_entry *ftn;

  /* Forward ilm entry */
  struct ilm_entry *ilm;

  /* Backward ilm entry */
  struct ilm_entry *ilm_b;
};

struct ilm_del_gen_data
{
  u_int32_t iif_ix;
  struct gmpls_gen_label in_label;
  u_int32_t ilm_ix;
  u_int16_t flags;
};

/* For API's which do an add and delete in both directions at the same time */
struct ftn_bidir_add_data
{
  /* FTN message ID.  */
  u_int32_t id;

  struct ftn_add_gen_data ftn;
  struct ilm_add_gen_data ilm;
};

struct ilm_bidir_add_data
{
  u_int32_t id;

  struct ilm_add_gen_data ilm_fwd;
  struct ilm_add_gen_data ilm_bwd;
};

struct ftn_bidir_ret_data
{
  u_int32_t id;

  struct ftn_ret_data ftn;
  struct ilm_gen_ret_data ilm;
};

struct ilm_bidir_ret_data
{
  u_int32_t id;

  struct ilm_gen_ret_data ilm_fwd;
  struct ilm_gen_ret_data ilm_bwd;
};

struct ftn_bidir_del_data
{
  u_int32_t id;

  struct ftn_del_gen_data ftn;
  struct ilm_del_gen_data ilm;
};

struct ilm_bidir_del_data
{
  struct ilm_del_gen_data ilm_fwd;
  struct ilm_del_gen_data ilm_bwd;
};

/* Global FTN table identifier. */
#define GLOBAL_FTN_ID          0

#ifdef HAVE_MPLS_VC
/* Virtual Circuit max key len. */
#define VC_KEY_LEN             64

/* VC Types. */
#define VC_TYPE_ETH_VLAN       0x0004
#define VC_TYPE_ETHERNET       0x0005
#define VC_TYPE_HDLC           0x0006
#define VC_TYPE_PPP            0x0007
#ifdef HAVE_ATM_VC
#define VC_TYPE_ATM_N2ONE_VCC            0x0009
#define VC_TYPE_ATM_N2ONE_VPC            0x000A
#endif /* HAVE_ATM_VC */
#ifdef HAVE_TDM_VC
#define VC_TYPE_E1_SATOP       0x0011
#define VC_TYPE_T1_SATOP       0x0012
#define VC_TYPE_CESOPSN_BASIC  0x0015
#endif /* HAVE_TDM_VC */
#define VC_TYPE_UNKNOWN        0xFFFF

#define VPLS_ID_LEN            32

/* Valid VC type check. */
#define MPLS_VC_IS_VALID(t) \
          (((t) == VC_TYPE_ETH_VLAN) || ((t) == VC_TYPE_ETHERNET) || \
                    ((t) == VC_TYPE_PPP) || \
                    ((t) == VC_TYPE_HDLC)) ? PAL_TRUE : PAL_FALSE
#endif /* HAVE_MPLS_VC */

#if 0
                    ((t) == VC_TYPE_T1_SATOP) || \
                    ((t) == VC_TYPE_E1_SATOP) || \
                    ((t) == VC_TYPE_CESOPSN_BASIC) || \
                    ((t) == VC_TYPE_ATM_N2ONE_VCC) || \
                    ((t) == VC_TYPE_ATM_N2ONE_VPC)) \
                    ? PAL_TRUE : PAL_FALSE
#endif /* needs to be fixed GKD */

#define MPLS_VC_TYPE_IS_ETHERNET(t) \
           (((t) == VC_TYPE_ETH_VLAN) || ((t) == VC_TYPE_ETHERNET) || \
                     ((t) == VC_TYPE_PPP) || \
                     ((t) == VC_TYPE_HDLC)) ? PAL_TRUE : PAL_FALSE

#define MPLS_VC_TYPE_IS_ETHERNET_VLAN(t) \
           ((t) == VC_TYPE_ETH_VLAN) ? PAL_TRUE : PAL_FALSE

#ifdef HAVE_TDM_VC
#define MPLS_VC_TYPE_IS_TDM(t) \
           (((t) == VC_TYPE_E1_SATOP) || ((t) == VC_TYPE_T1_SATOP) ||  \
                      ((t) == VC_TYPE_CESOPSN_BASIC)) ? PAL_TRUE : PAL_FALSE
#endif /* HAVE_TDM_VC */
#ifdef HAVE_ATM_VC
#define MPLS_VC_TYPE_IS_ATM(t) \
    +          (((t) == VC_TYPE_ATM_N2ONE_VPC) || ((t) == VC_TYPE_ATM_N2ONE_VCC)) ?  PAL_TRUE : PAL_FALSE
#endif /* HAVE_ATM_VC */

#define CLASS_TO_DSCP(class,dscp)                                     \
        do {                                                          \
             int k;                                                   \
                                                                      \
             (dscp) = DIFFSERV_INVALID_DSCP;                          \
             if (pal_strstr ((class), "undefined"))                   \
               break;                                                 \
                                                                      \
             for (k = 0; k < DIFFSERV_MAX_SUPPORTED_DSCP; k++)        \
               if (pal_strcmp ((class), diffserv_class_name(k)) == 0) \
                 {                                                    \
                   (dscp) = k;                                        \
                   break;                                             \
                 }                                                    \
           } while (0)

/* Diffserv defines. */
#define DIFFSERV_MAX_SUPPORTED_DSCP      64
#define DIFFSERV_INVALID_DSCP            64

#define MAX_BW_CONST            8
#define MAX_CLASS_TYPE          8
#define MAX_TE_CLASS            8

#define NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY(G, P)                            \
   (G)->type = gmpls_entry_type_ip;                                           \
   prefix_copy (&(G)->u.prefix, P);                                           \

#define NSM_MPLS_GET_FEC_PREFIX4_FROM_FTN(F, P)         \
do {                                                    \
   u_char byte_len;                                     \
                                                        \
   (P).family = AF_INET;                                \
   (P).prefixlen = (F)->pn->key_len;                    \
   byte_len = (F)->pn->key_len / 8;                     \
   if ((F)->pn->key_len % 8)                            \
     byte_len ++;                                       \
   pal_mem_cpy (&(P).prefix, (F)->pn->key, byte_len);   \
                                                        \
} while (0)                                             \

#ifdef HAVE_IPV6
#define NSM_MPLS_GET_FEC_PREFIX6_FROM_FTN(F, P)         \
do {                                                    \
   u_char byte_len;                                     \
                                                        \
   (P).family = AF_INET6;                               \
   (P).prefixlen = (F)->pn->key_len;                    \
   byte_len = (F)->pn->key_len / 8;                     \
   if ((F)->pn->key_len % 8)                            \
     byte_len ++;                                       \
   pal_mem_cpy (&(P).prefix, (F)->pn->key, byte_len);   \
                                                        \
} while (0)                                             \

#endif /* HAVE_IPV6 */

/* Function prototypes */
char *mpls_dump_owner (mpls_owner_t);
char *mpls_dump_owner_char (mpls_owner_t);
char *mpls_dump_action_type (ftn_action_type_t);
char *nsm_mpls_dump_op_code (u_char);
char *nsm_mpls_dump_notification_type (mpls_notification_t);
char *nsm_mpls_dump_notification_status (mpls_status_code_t);
char *diffserv_class_name (u_char);
int  gmpls_addr_in_same (struct addr_in *, struct addr_in *);
int  gmpls_addr_in_zero (struct addr_in *);
int  mpls_addr_in_to_prefix (struct addr_in *, struct prefix *);
int mpls_addr_in_to_str (struct addr_in *, char *);
int  mpls_owner_same (struct mpls_owner *, struct mpls_owner *);
mpls_owner_t gmpls_proto_to_owner (u_int32_t);
u_int32_t gmpls_owner_to_proto (mpls_owner_t);
bool_t gmpls_are_label_equal (struct gmpls_gen_label *lbl1,
                              struct gmpls_gen_label *lbl2);

#ifdef HAVE_MPLS_VC
const char *mpls_vc_type_to_str (u_int16_t);
const char *mpls_vc_type_to_str2 (u_int16_t);
#endif /* HAVE_MPLS_VC */

#endif /* _PACOS_MPLS_H */
