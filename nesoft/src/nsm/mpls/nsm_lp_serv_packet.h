/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_LP_SERV_PACKET_H
#define _PACOS_LP_SERV_PACKET_H

#include "nsm_lp_serv.h"


int
nsm_lp_server_pkt_max_blocks (struct lp_server *);

struct lp_server*
nsm_lp_server_pkt_new (struct nsm_master *, u_int16_t, u_int8_t,
                    enum gmpls_label_type );

struct lp_server *
nsm_lp_server_pkt_get (struct nsm_master *, u_int16_t ,
                   int, u_int8_t, enum gmpls_label_type );

int
nsm_lp_server_pkt_find_next_free (struct lp_server *server);

u_int32_t
nsm_lp_server_pkt_get_block (struct nsm_master *nm,
                         u_int16_t label_space,
                         u_char protocol,
                         struct nsm_msg_pkt_block_list *blist,
                         struct lp_serv_lset_block *ls_blk,
                         u_int8_t *range_ret_data,
                         u_int8_t range_owner,
                         enum gmpls_label_type  lbl_format);
void
nsm_lp_server_pkt_free (struct nsm_master *nm, struct route_node *rn,
                    enum gmpls_label_type  lbl_format);

int
nsm_lp_server_pkt_free_block (struct nsm_master *nm, u_int16_t label_space,
                         u_int32_t block, u_char protocol,
                         u_int8_t range_owner, enum gmpls_label_type  lbl_format);

u_int32_t
nsm_lp_server_pkt_get_label_value (struct nsm_master *nm,
                               u_int16_t label_space, int get_maximum,
                               u_int8_t range_owner,
                               enum gmpls_label_type  lbl_format);

int
nsm_lp_server_pkt_in_use (struct nsm_master *nm, u_int32_t label_space,
                      u_int8_t range_owner,
                      enum gmpls_label_type  lbl_format);

void
nsm_lp_server_pkt_clean_for_w_status (struct nsm_master *nm, u_char protocol,
                                u_char status, u_int8_t range_owner,
                                enum gmpls_label_type  lbl_format);

void
nsm_lp_server_pkt_clean_in_use_for (struct nsm_master *nm, u_char protocol,
                               u_int8_t range_owner,
                               enum gmpls_label_type  lbl_format);

void
nsm_lp_server_pkt_clean_stale_for (struct nsm_master *nm, u_char protocol,
                             u_int8_t range_owner,
                             enum gmpls_label_type  lbl_format);

void
nsm_lp_server_pkt_set_stale_for (struct nsm_master *nm, u_char protocol,
                           enum gmpls_label_type  lbl_format);


void
nsm_lp_server_pkt_get_stale_for (struct nsm_master *nm,
                             struct nsm_msg_service *message,
                             u_char protocol,
                             struct nsm_msg_label_pool **label_pools,
                             u_int16_t *label_pool_num,
                             u_int8_t range_owner,
                             enum gmpls_label_type  lbl_format);

void
nsm_lp_server_pkt_init_ranges (struct nsm_master *nm,
                          u_int16_t label_space,
                          struct lp_server *server);

int
nsm_lp_server_pkt_process_lset_req (struct nsm_master *nm,
                                  u_int16_t label_space,
                                  struct lp_server *server,
                                  struct nsm_msg_pkt_block_list *blist,
                                  struct lp_serv_lset_block *ls_blk,
                                  u_int8_t service_owner,
                                  int service_index,
                                  u_char protocol);
u_int32_t
nsm_lp_server_pkt_get_range_based_block (struct nsm_master *nm,
                                    u_int16_t label_space,
                                    struct lp_server *server,
                                    u_int8_t service_owner,
                                    int service_index,
                                    u_char protocol,
                                    u_int8_t *range_ret_data);
int
nsm_lp_server_pkt_get_lset_block (struct nsm_master *nm,
                                u_int16_t label_space,
                                struct lp_server *server,
                                struct nsm_msg_pkt_block_list *blist,
                                struct lp_serv_lset_block *ls_blk,
                                u_int8_t service_owner,
                                int service_index,
                                u_int32_t *ret_lbl,
                                u_char protocol);

int
nsm_lp_server_pkt_range_find_next_free(struct nsm_master *nm,
                                 u_int16_t label_space,
                                 struct lp_server *server,
                                 u_int8_t service_owner,
                                 int service_index);
int
nsm_lp_server_pkt_free_range_based_block(struct lp_server *server,
                                    u_int8_t range_owner,
                                    u_int32_t block);
u_int32_t
nsm_lp_server_pkt_get_range_label_value (struct nsm_master *nm,
                                    u_int16_t label_space,
                                    int get_maximum,
                                    u_int8_t range_owner,
                                    enum gmpls_label_type  lbl_format);
#endif /* _PACOS_LP_SERV_PACKET_H */

