/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_LP_SERV_H
#define _PACOS_LP_SERV_H

#include "label_pool.h"
#include "rib/nsm_table.h"

#define LP_SERVER_BLOCK_AVAILABLE       0
#define LP_SERVER_BLOCK_IN_USE          1
#define LP_SERVER_BLOCK_STALE           2

struct lp_block
{
  u_char    status;
  u_char    protocol;
  u_int8_t  range_owner;  /* Owner in terms of client service type */
};

struct lp_server
{
  /* The labelspace corresponding to this label pool server */
  u_int16_t label_space;

  /* The following array is used to identify the blocks
     in use for the given label-space */
#ifdef HAVE_PACKET
  struct lp_block lp_block[LABEL_VALUE_MAX / LABEL_BUCKET_SIZE];
#endif

#ifdef HAVE_PORT_WAVELENGTH
  struct lp_block lp_block_port_wlength[LABEL_VALUE_MAX /
                                                WAVELENGTH_LABEL_BUCKET_SIZE];
#endif

#ifdef HAVE_FREEFORM
  struct lp_block lp_block_freeform[LABEL_VALUE_MAX/FREEFORM_LABEL_BUCKET_SIZE];
#endif

#ifdef HAVE_SONET
  struct lp_block lp_block_sonet[LABEL_VALUE_MAX / SONET_LABEL_BUCKET_SIZE];
#endif

#ifdef HAVE_SDH
  struct lp_block lp_block_sdh[LABEL_VALUE_MAX / SDH_LABEL_BUCKET_SIZE];
#endif

#ifdef HAVE_WAVEBAND
  struct lp_block lp_block_waveband[LABEL_VALUE_MAX/WAVEBAND_LABEL_BUCKET_SIZE];
#endif

#ifdef HAVE_TDM
  struct lp_block lp_block_tdm[LABEL_VALUE_MAX / TDM_LABEL_BUCKET_SIZE];
#endif

  /* Minimum label value */
  u_int32_t min_label_value;

  /* Maximum label value */
  u_int32_t max_label_value;

  /* The following denotes the first free block for flat label space */
  int first_free_block;

  /* Counter for the number of free blocks in flat label space */
  int total_free_blocks;
  /* The following denotes the first free block in case of range based labels */
  int first_free_range_block[LABEL_POOL_RANGE_MAX];

  /* Counter for the number of free blocks in case of range based labels */
  int total_free_range_blocks[LABEL_POOL_RANGE_MAX];

  /* Back pointer to the route node which stores this server */
  struct route_node *rn;
};

struct lp_serv_lset_block
{
  u_int8_t    lset_blk_type;
  u_int32_t   block_id;
};

/* Prototypes. */
int nsm_lp_server_free_block (struct nsm_master *, u_int16_t, u_int32_t,
                              u_char);
int nsm_lp_server_in_use (struct nsm_master *, u_int32_t);
void nsm_lp_server_clean_in_use_for (struct nsm_master *, u_char);
void nsm_lp_server_set_stale_for (struct nsm_master *, u_char);
void nsm_lp_server_clean_stale_for (struct nsm_master *, u_char);
void nsm_lp_server_get_stale_for (struct nsm_master *,
                                  struct nsm_msg_service *,
                                  u_char,
                                  struct nsm_msg_label_pool **,
                                  u_int16_t *);
u_int32_t nsm_lp_server_get_block (struct nsm_master *, u_int16_t, u_char);
u_int32_t nsm_lp_server_get_label_value (struct nsm_master *, u_int16_t, int);
u_int32_t nsm_lp_static_block_num_get (u_int32_t, u_int32_t);
void nsm_lp_static_block_get (struct nsm_master *, struct nsm_label_space *,
                              u_char);
void nsm_lp_static_block_release (struct nsm_master *, struct nsm_label_space *,
                                  u_char);
u_int32_t nsm_lp_server_static_block_num_get (u_int32_t, u_int32_t);

void nsm_lp_server_static_block_get (struct nsm_master *,
                                struct nsm_label_space *,
                                u_char protocol);
void nsm_lp_server_static_block_release (struct nsm_master *,
                        struct nsm_label_space *, u_char);

int
nsm_lp_server_get_service_range_index (u_int8_t);

u_int8_t
nsm_lp_server_get_service_range_type (int);

/******************************************************************************
 *
 *               GMPLS LABEL POOL SERVER SECTION
 *
 ******************************************************************************/

int nsm_lp_server_gen_max_blocks (struct lp_server *server,
                           enum gmpls_label_type  lbl_format);

struct lp_server*nsm_lp_server_gen_new (struct nsm_master *nm,
                     u_int16_t label_space, u_int8_t range_owner,
                     enum gmpls_label_type  lbl_format);

struct lp_server * nsm_lp_server_gen_get (struct nsm_master *nm,
                    u_int16_t label_space,
                    int create, u_int8_t range_owner,
                    enum gmpls_label_type  lbl_format);
int nsm_lp_server_gen_find_next_free (struct lp_server *server,
                             enum gmpls_label_type  lbl_format);

u_int32_t nsm_lp_server_gen_get_block (struct nsm_master *nm,
                          u_int16_t label_space,
                          u_char protocol,
                          struct nsm_msg_pkt_block_list *blist,
                          struct lp_serv_lset_block *ls_blk,
                          u_int8_t *range_ret_data,
                          u_int8_t range_owner,
                          enum gmpls_label_type  lbl_format);

u_int32_t nsm_lp_server_gen_static_block_num_get (u_int32_t min,
                         u_int32_t max, enum gmpls_label_type  lbl_format);


void nsm_lp_server_gen_static_block_get (struct nsm_master *nm,
                        struct nsm_label_space *label_space,
                        u_char protocol, u_int8_t range_owner,
                        enum gmpls_label_type  lbl_format);

void nsm_lp_server_gen_free (struct nsm_master *nm,
                     struct route_node *rn,
                     enum gmpls_label_type  lbl_format);

int nsm_lp_server_gen_free_block (struct nsm_master *nm,
                          u_int16_t label_space,
                          u_int32_t block,
                          u_char protocol,
                          u_int8_t service_owner,
                          enum gmpls_label_type  lbl_format);
void
nsm_lp_server_gen_static_block_release (struct nsm_master *nm,
                            struct nsm_label_space *nls,
                            u_char protocol,
                            enum gmpls_label_type  lbl_format);

void nsm_lp_gen_static_block_release (struct nsm_master *nm,
                            struct nsm_label_space *nls,
                            u_char protocol,
                            enum gmpls_label_type  lbl_format);

u_int32_t nsm_lp_server_gen_get_label_value (struct nsm_master *nm,
                              u_int16_t label_space,
                              int get_maximum,
                              u_int8_t service_owner,
                              enum gmpls_label_type  lbl_format);

u_int32_t nsm_lp_server_gen_get_range_label_value (struct nsm_master *nm,
                                    u_int16_t label_space,
                                    int get_maximum,
                                    u_int8_t service_owner,
                                    enum gmpls_label_type  lbl_format);

int nsm_lp_server_gen_in_use (struct nsm_master *nm,
                          u_int32_t label_space,
                          u_int8_t range_owner,
                          enum gmpls_label_type  lbl_format);

void nsm_lp_server_gen_clean_for_w_status (struct nsm_master *nm,
                                 u_int16_t label_space,
                                 u_char status,
                                 u_int8_t service_owner,
                                 enum gmpls_label_type  lbl_format);

void nsm_lp_server_gen_clean_in_use_for (struct nsm_master *nm,
                                u_char protocol,
                                u_int8_t range_owner,
                                enum gmpls_label_type  lbl_format);

void nsm_lp_server_gen_clean_stale_for (struct nsm_master *nm,
                              u_char protocol,
                              u_int8_t range_owner,
                              enum gmpls_label_type  lbl_format);

void nsm_lp_server_gen_set_stale_for (struct nsm_master *nm,
                            u_char protocol, u_int8_t range_owner,
                            enum gmpls_label_type  lbl_format);

void nsm_lp_server_gen_get_stale_for (struct nsm_master *nm,
                                 struct nsm_msg_service *message,
                                 u_char protocol,
                                 struct nsm_msg_label_pool **label_pools,
                                 u_int16_t *label_pool_num,
                                 u_int8_t range_owner,
                                 enum gmpls_label_type  lbl_format);

/* End Prototypes */

#endif /* _PACOS_LP_SERV_H */
