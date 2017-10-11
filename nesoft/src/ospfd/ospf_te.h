/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_OSPF_TE_H
#define _PACOS_OSPF_TE_H

#ifdef HAVE_OSPF_TE

#ifndef HAVE_OPAQUE_LSA
#error "Wrong ospfd config."
#endif /* HAVE_OPAQUE_LSA */

#ifdef HAVE_GMPLS
#define OSPF_TE_LINK_TABLE_DEPTH             20
#endif /* HAVE_GMPLS */

#ifdef HAVE_OSPF_CSPF

/* Link TLV flags */
#define TE_LINK_ATTR_LINK_TYPE              (1 << 0)
#define TE_LINK_ATTR_LINK_ID                (1 << 1)
#define TE_LINK_ATTR_LOCAL_IF_ADDR          (1 << 2)
#define TE_LINK_ATTR_REMOTE_IF_ADDR         (1 << 3)
#define TE_LINK_ATTR_TE_METRIC              (1 << 4)
#define TE_LINK_ATTR_MAX_BANDWIDTH          (1 << 5)
#define TE_LINK_ATTR_MAX_RES_BANDWIDTH      (1 << 6)
#define TE_LINK_ATTR_UNRSV_BANDWIDTH        (1 << 7)
#define TE_LINK_ATTR_RSRC_COLOR             (1 << 8)
#define TE_LINK_ATTR_BC                     (1 << 9)
#define TE_LINK_ATTR_NEIGHBOR_ID            (1 << 10) 
#define TE_LINK_ATTR_LOCAL_IF_IPV6_ADDR     (1 << 11)
#define TE_LINK_ATTR_REMOTE_IF_IPV6_ADDR    (1 << 12)
#ifdef HAVE_GMPLS
#define TE_LINK_ATTR_LINK_LOCAL_REMOTE_ID   (1 << 13)
#define TE_LINK_ATTR_SWITCHING_CAP          (1 << 14)
#define TE_LINK_ATTR_PROTECTION             (1 << 15)
#define TE_LINK_ATTR_SRLG                   (1 << 16)
#endif /* HAVE_GMPLS*/

#define OSPF_TE_LSA_MASK                    0xff000000
#define OSPF_TE_LSA_VALUE                   0x01000000

#define IS_OSPF_TE_LSA(lsa) \
  (((lsa)->data->type == OSPF_AREA_OPAQUE_LSA) && \
   ((pal_ntoh32 ((lsa)->data->id.s_addr) & OSPF_TE_LSA_MASK) == OSPF_TE_LSA_VALUE))

struct ospf_te_tlv_link_type
{
  struct te_tlv_header tlv_h;
  u_char link_type;
};

struct ospf_te_tlv_link_id
{
  struct te_tlv_header tlv_h;
  struct pal_in4_addr link_id;
};

struct ospf_te_tlv_if_addr
{
  struct te_tlv_header tlv_h;
  struct pal_in4_addr *addr;
};

struct ospf_te_tlv_metric
{
  struct te_tlv_header tlv_h;
  u_int32_t te_metric;
};

struct ospf_te_tlv_bw
{
  struct te_tlv_header tlv_h;
  float32_t bw;
};

struct ospf_te_tlv_unrsv_bw
{
  struct te_tlv_header tlv_h;
  float32_t bw[MAX_PRIORITIES];
};

struct ospf_te_tlv_rsrc_color
{
  struct te_tlv_header tlv_h;
  u_int32_t color;
};

#ifdef HAVE_DSTE
struct ospf_te_tlv_bc
{
  struct te_tlv_header tlv_h;
  u_char bc_model;
  float32_t bc[MAX_PRIORITIES];
};
#endif /* HAVE_DSTE */

#ifdef HAVE_GMPLS
/*Link Local/Remote Identifier TLV */
struct ospf_te_tlv_local_remote_id
{
  struct te_tlv_header tlv_h;
  struct pal_in4_addr local_if_id;
  struct pal_in4_addr remote_if_id;
};

/*Switching Capability Descriptor TLV */
struct ospf_te_tlv_capability
{
  struct te_tlv_header tlv_h;
  u_int8_t capability;
  u_int8_t encoding;
  u_int16_t reserve;
  float32_t max_lsp_bw[MAX_PRIORITIES];
#ifdef HAVE_PACKET
  float32_t min_lsp_bw;
  u_int16_t mtu;
  u_int16_t pad;
#endif /* HAVE_PACKET */
};

/*Link Protection TLV */
struct ospf_te_tlv_protection
{
  struct te_tlv_header tlv_h;
  u_int8_t protection;
  u_int8_t reserve[3];
};

#define OSPF_SRLG_VALUE_SIZE      4

/*Shared Risk Group TLV */
struct ospf_te_tlv_srlg
{
  struct te_tlv_header tlv_h;
  u_int32_t *srlg;
};
#endif /* HAVE_GMPLS */

struct ospf_te_tlv_link_lsa
{
  struct te_tlv_header tlv_h;
  u_int32_t flag;
  u_int32_t number_sw_cap;

  struct ospf_te_tlv_link_type link_type;
  struct ospf_te_tlv_link_id link_id;
  struct ospf_te_tlv_if_addr local_addr;
  struct ospf_te_tlv_if_addr remote_addr;
  struct ospf_te_tlv_metric te_metric;
  struct ospf_te_tlv_bw max_bw;
  struct ospf_te_tlv_bw max_res_bw;
  struct ospf_te_tlv_unrsv_bw unrsv_bw;
  struct ospf_te_tlv_rsrc_color color;
#ifdef HAVE_DSTE
  struct ospf_te_tlv_bc bc;
#endif /* HAVE_DSTE */
#ifdef HAVE_GMPLS
  struct ospf_te_tlv_local_remote_id local_remote_id;
  struct ospf_te_tlv_capability *cap;
  struct ospf_te_tlv_protection protection;
  struct ospf_te_tlv_srlg srlg;
#endif /* HAVE_GMPLS */
};

#endif /* HAVE_OSPF_CSPF */

/* OSPF TE Link */
struct ospf_telink
{
  /* Back pointer to the OSPF instance structure */
  struct ospf *top;

  /* Area where this TE information will be flooded */
  struct ospf_area *area;

  /* Back pointer to TE Link structure */
  struct telink *tlink;

  /* Configuration parameters. */
  struct ospf_tlink_params *params;
};

struct ospf_tlink_params
{
  /* TE Link name. */
  char *desc;

  /* Configured flags. */
  u_int32_t config;
#define OSPF_TLINK_PARAM_AREA_ID                   (1 << 0)
#ifdef HAVE_OSPF_TE
#define OSPF_TLINK_PARAM_TE_METRIC                 (1 << 1)
#endif /* HAVE_OSPF_TE */
#define OSPF_TLINK_PARAM_TE_LINK_LOCAL             (1 << 2)

  /* OSPF process ID. */
  int proc_id;

  /* OSPF area ID. */
  struct pal_in4_addr area_id;

  /* OSPF area ID format. */
  u_char format;

#ifdef HAVE_OSPF_TE
  /* TE metric */
  u_int32_t te_metric;
#endif /* HAVE_OSPF_TE */
};

#ifdef HAVE_OSPF_CSPF

struct ospf_te_tlv_rtaddr_lsa
{
  struct te_tlv_header tlv_h;
  struct pal_in4_addr router_id;
};

struct ospf_te_lsa_data
{
  struct lsa_header header;
  u_char type;
  union
  {
    struct ospf_te_tlv_link_lsa link_lsa;
    struct ospf_te_tlv_rtaddr_lsa rtaddr_lsa;
  } ospf_te_tlv_lsa;
};

int ospf_te_tlv_header_decode (u_char **, u_int16_t *,
                               struct te_tlv_header *);
void ospf_te_lsa_header_decode (u_char **, u_int16_t *, struct lsa_header *);
int ospf_te_lsa_decode (u_char **, u_int16_t *, struct ospf_te_lsa_data *);
int ospf_te_tlv_rtaddr_lsa_decode (u_char **, u_int16_t *,
                                   struct te_tlv_header *,
                                   struct ospf_te_tlv_rtaddr_lsa *);
int ospf_te_tlv_link_lsa_decode (u_char **, u_int16_t *,
                                 struct te_tlv_header *,
                                 struct ospf_te_tlv_link_lsa *);
int ospf_te_tlv_rsrc_color_decode (u_char **, u_int16_t *,
                                   struct te_tlv_header *,
                                   struct ospf_te_tlv_rsrc_color *);
int ospf_te_tlv_unrsv_bw_decode (u_char **, u_int16_t *,
                                 struct te_tlv_header *,
                                 struct ospf_te_tlv_unrsv_bw *);
int ospf_te_tlv_bw_decode (u_char **, u_int16_t *,
                           struct te_tlv_header *,
                           struct ospf_te_tlv_bw *);
int ospf_te_tlv_metric_decode (u_char **, u_int16_t *,
                               struct te_tlv_header *,
                               struct ospf_te_tlv_metric *);
int ospf_te_tlv_if_addr_decode (u_char **, u_int16_t *,
                                struct te_tlv_header *,
                                struct ospf_te_tlv_if_addr *);
int ospf_te_tlv_link_id_decode (u_char **, u_int16_t *,
                                struct te_tlv_header *,
                                struct ospf_te_tlv_link_id *);
int ospf_te_tlv_link_type_decode (u_char **, u_int16_t *,
                                  struct te_tlv_header *_h,
                                  struct ospf_te_tlv_link_type *);
#ifdef HAVE_DSTE
int ospf_te_tlv_bc_decode (u_char **, u_int16_t *,
           struct te_tlv_header *, struct ospf_te_tlv_bc *);
#endif /* HAVE_DSTE */

#ifdef HAVE_GMPLS
int ospf_te_tlv_local_remote_id_decode (u_char **, u_int16_t *,
                                     struct te_tlv_header *,
                                     struct ospf_te_tlv_local_remote_id *);
int ospf_te_tlv_capability_decode (u_char **, u_int16_t *,
                                  struct te_tlv_header *,
                                  struct ospf_te_tlv_capability *);
int ospf_te_tlv_protection_decode (u_char **, u_int16_t *,
                                  struct te_tlv_header *,
                                  struct ospf_te_tlv_protection *);
int ospf_te_tlv_srlg_decode (u_char **, u_int16_t *,
                             struct te_tlv_header *,
                             struct ospf_te_tlv_srlg *);
#endif /* HAVE_GMPLS */

int ospf_te_tlv_unknown_decode (u_char **, u_int16_t *,
                                struct te_tlv_header *);

struct ospf_lsa *ospf_te_lsa_new (struct ospf_lsa *,
                                  struct ospf_te_lsa_data **);
void ospf_te_lsa_free (struct ospf_lsa *);
void ospf_te_lsa_data_free (struct ospf_te_lsa_data *);
void ospf_te_process_lsa_add (struct ospf_master *, struct ptree *,
                              struct ospf_lsa *);
void ospf_te_process_lsa_delete (struct ospf_master *,
                                 struct ptree *, struct ospf_lsa *);

#endif /* HAVE_OSPF_CSPF */

int ospf_te_if_attr_update (struct interface *, struct ospf *);
void ospf_te_withdraw_link (struct ospf *, struct interface *);
void ospf_te_announce_te_link (struct ospf_telink *);
void ospf_te_link_local_lsa_set (struct ospf_telink *);
void ospf_te_link_local_lsa_unset (struct ospf_telink *);
struct ospf_telink *ospf_te_link_new ( u_int32_t, struct telink *);
void ospf_activate_te_link (struct ospf_telink *);
void ospf_deactivate_te_link (struct ospf_telink *);
void ospf_update_te_link (struct ospf_telink *);
int ospf_te_show_opaque_data (struct cli *, struct opaque_lsa *);
#ifdef HAVE_GMPLS
void ospf_te_if_gmpls_type_update (struct ospf_interface *);
void ospf_telink_table_init (struct ospf_master *);
void ospf_telink_table_finish (struct ospf_master *);
#endif /* HAVE_GMPLS */
int ospf_te_init (struct ospf_master *);
void ospf_te_withdraw_lsa (struct ospf *);
#ifdef HAVE_GMPLS
struct ospf_tlink_params *ospf_tlink_params_new ();
#endif /* HAVE_GMPLS */

#endif /* HAVE_OSPF_TE */

#endif /* _PACOS_OSPF_TE_H */
