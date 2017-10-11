/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_LP_CLIENT_PACKET_PACKET_H
#define _PACOS_LP_CLIENT_PACKET_PACKET_H

#include "lp_client.h"

/******************************************************************************/
/*                                                                            */
/*             Label Pool Client header file for Packet technology            */
/*                                                                            */
/******************************************************************************/

/* lp_client_pkt_request_label_by_value flags */
#define LP_CLIENT_PKT_FLAG_NONE               0
#define LP_CLIENT_PKT_REQ_LABEL_FROM_SERVER   (1 << 0)

/* Label set object lists based on packet labels, action type for internal use*/
struct lp_client_pkt_lso
{
  u_int32_t  pkt_label_count;
  u_int32_t *pkt_labels;

  struct   lp_client_pkt_lso *next;
  struct   lp_client_pkt_lso *prev;
};

struct lp_client_pkt_lso_list
{
  /* Count of number of lists */
  u_int32_t                   count;

  struct   lp_client_pkt_lso *head;
  struct   lp_client_pkt_lso *tail;
};

#define LP_CLIENT_PKT_ONE_ELEM 1

struct lp_client_pkt_lso_blk
{
  u_int32_t pkt_block;

  /* Start of valid label for this block */
  u_int32_t start_label;

  /* Number of labels valid from the start */
  u_int32_t lbl_count;

  struct   lp_client_pkt_lso_blk *next;
  struct   lp_client_pkt_lso_blk *prev;
};

struct lp_client_pkt_lso_blk_list
{
  u_int8_t                        action_type;
  u_int8_t                        count;
  struct   lp_client_pkt_lso_blk *head;
  struct   lp_client_pkt_lso_blk *tail;
};

#define LP_CLIENT_PKT_LSO_ADD_TAIL(L,N)               \
  do {                                                \
    (N)->prev = (L)->tail;                            \
    (N)->next = NULL;                                 \
    if ((L)->head)                                    \
      (L)->tail->next = (N);                          \
    else                                              \
      (L)->head = (N);                                \
    (L)->tail = (N);                                  \
    (L)->count++;                                     \
  } while (0)

#define LP_CLIENT_PKT_LSO_DEL(L,N)                    \
  do {                                                \
    if ((N)->prev)                                    \
      (N)->prev->next = (N)->next;                    \
    else                                              \
      (L)->head = (N)->next;                          \
    if ((N)->next)                                    \
      (N)->next->prev = (N)->prev;                    \
    else                                              \
      (L)->tail = (N)->prev;                          \
    (L)->count--;                                     \
  } while (0)

/* Prototypes */
/*============*/
struct lp_client * lp_client_pkt_new (struct lib_globals *,
                  struct route_table *,u_int16_t, u_int32_t, u_int32_t,
                  enum label_pool_id);

void lp_client_pkt_init (struct lib_globals *, struct route_table **);

struct bitmap_block* lp_client_pkt_get_block (struct lib_globals *,
                                         struct route_table *,
                                         u_int16_t, u_int32_t, u_int32_t *,
                                         enum label_pool_id);

int lp_client_pkt_request_label (struct lib_globals *, struct route_table *,
                                 u_int16_t, struct nsm_client *,
                                 enum label_pool_id);

int lp_client_pkt_release_label (struct lib_globals *, struct route_table *,
                            u_int16_t, u_int32_t label,
                            struct nsm_client *, enum label_pool_id);

int lp_client_pkt_request_block_send (struct lib_globals *, struct route_table *,
                                u_int16_t, struct nsm_client *,
                                struct nsm_msg_pkt_block_list *,
                                enum label_pool_id);

int lp_client_pkt_release_block_send (struct lib_globals *, struct route_table *,
                                u_int16_t, u_int32_t, struct nsm_client *,
                                int, enum label_pool_id);

int lp_client_pkt_release_label_cleanup (struct lib_globals *, struct route_table *,
                                   u_int16_t, u_int32_t, struct nsm_client *,
                                   int, enum label_pool_id);

void lp_client_pkt_free (struct lib_globals *, struct route_table *,
                     struct nsm_client *, enum label_pool_id);

struct lp_client* lp_client_pkt_lookup (struct lib_globals *,struct route_table *,
                                               u_int16_t, enum label_pool_id);

int lp_client_pkt_init_label_block (struct lib_globals *, struct route_table *,
                          u_int16_t, struct nsm_client *, enum label_pool_id);

void lp_client_pkt_deinit (struct lib_globals *, struct route_table *, u_int16_t,
                         struct nsm_client *, enum label_pool_id);


u_int32_t lp_client_pkt_get_available_label_count (struct lib_globals *,
                                            struct route_table *, u_int16_t,
                                            u_int32_t, enum label_pool_id);

int lp_client_pkt_is_usable (struct lib_globals *, struct route_table *, u_int16_t,
                        u_int32_t, enum label_pool_id);

int lp_client_pkt_bind_block (struct lib_globals *, struct route_table *,
                            u_int16_t, u_int32_t, u_int32_t, u_int32_t,
                            enum label_pool_id);

int lp_client_pkt_release_block (struct lib_globals *, struct route_table *,
                           u_int16_t, u_int32_t, enum label_pool_id);

int lp_client_gen_reserve_suggested_label (struct lib_globals *zg,
                                     struct route_table *table,
                                     u_int16_t lbl_space,
                                     struct nsm_client *nc,
                                     struct generalized_label* gsugg_label,
                                     enum label_pool_id serv_module);

int lp_client_pkt_set_service_type(struct lib_globals *, struct route_table *,
                             u_int16_t, enum label_pool_id);

void lp_client_pkt_block_list_free (struct lib_globals *, struct route_table *,
                             struct lp_client *, struct nsm_client *,
                             enum label_pool_id);

void lp_client_pkt_lset_free (struct lib_globals *, struct route_table *,
                        struct nsm_client *, struct lp_client_pkt_lso_list *,
                    struct lp_client_pkt_lso_blk_list *, enum label_pool_id);

int lp_client_pkt_preproc_lso_range (struct lib_globals *, struct lp_client*,
                          struct lp_client_glabel_list*,
                          struct lp_client_pkt_lso_list *, enum label_pool_id);

int lp_client_pkt_preproc_lso_list (struct lib_globals *, struct lp_client*,
                         struct lp_client_glabel_list*,
                         struct lp_client_pkt_lso_list *, enum label_pool_id);

int lp_client_pkt_get_label_from_label_set (struct lib_globals *,
                                    struct route_table *,
                                    u_int16_t, struct nsm_client *,
                                    struct lp_client_label_set *,
                                    struct generalized_label *,
                                    enum label_pool_id);

int lp_client_pkt_req_on_demand_block_send (struct lib_globals *,
                                    struct route_table *,
                                    u_int16_t, struct nsm_client *,
                                    struct lp_client_pkt_lso_list *,
                                    struct lp_client_pkt_lso_blk_list *,
                                    struct generalized_label *,
                                    u_int8_t, enum label_pool_id);

void lp_client_pkt_update_request_failure (struct lib_globals *, struct route_table *,
                                    struct lp_client *,u_char ,
                                    enum label_pool_id);

int lp_client_pkt_get_label_from_lset_blks (struct lib_globals *,
                                struct lp_client *,
                                struct nsm_msg_pkt_block_list *,
                                u_int8_t,
                                struct lp_client_pkt_lso_blk_list *,
                                struct lp_client_pkt_lso_list *,
                                struct generalized_label *,
                                enum label_pool_id);
int lp_client_pkt_get_label_from_blk (struct lib_globals *,
                             struct lp_client *,
                             struct nsm_msg_pkt_block_list *,
                             u_int8_t,
                             struct lp_client_pkt_lso_blk_list *,
                             struct generalized_label *,
                             enum label_pool_id);

int lp_client_pkt_lbl_set_compare (const void *, const void *);

int lp_client_pkt_add_blk_to_list(struct lp_client_pkt_lso_blk_list *,
                            u_int32_t *, u_int32_t, u_int32_t);

int lp_client_pkt_process_include_list (struct lib_globals *,
                                struct lp_client *,
                                struct lp_client_pkt_lso_list *,
                                struct lp_client_pkt_lso_blk_list *,
                                u_int32_t *, enum label_pool_id);

int lp_client_pkt_process_exclude_list (struct lib_globals *,
                                 struct lp_client *,
                                 struct lp_client_pkt_lso_list *,
                                 struct lp_client_pkt_lso_blk_list *,
                                 u_int32_t *, enum label_pool_id);

int lp_client_pkt_lset_get_range_intersection (struct lp_client *,
                                   struct lp_client_glabel_list *,
                                   u_int32_t *, u_int32_t *,
                                   enum label_pool_id);

bool_t lp_client_pkt_check_for_exclusion(u_int32_t *,
                                   struct lp_client_pkt_lso_list *, u_int8_t);


int lp_client_pkt_find_label_in_range (struct lib_globals *, struct lp_client *,
                             u_int32_t, u_int32_t,
                             struct lp_client_pkt_lso_list *,
                             struct lp_client_pkt_lso_blk_list *,
                             u_int8_t, u_int32_t *, enum label_pool_id);

int lp_client_pkt_process_include_range (struct lib_globals *,
                                   struct lp_client *,
                                   struct lp_client_pkt_lso_list *,
                                   struct lp_client_pkt_lso_blk_list *,
                                   u_int32_t *, enum label_pool_id);

int lp_client_pkt_process_exclude_range (struct lib_globals *,
                                   struct lp_client *,
                                   struct lp_client_pkt_lso_list *,
                                   struct lp_client_pkt_lso_blk_list *,
                                   u_int32_t *, enum label_pool_id);

int lp_client_pkt_get_label_by_value (struct lib_globals*, struct route_table*,
                        u_int16_t, struct nsm_client*, u_int8_t,
                        struct generalized_label*,
                        struct lp_client_label_set *, enum label_pool_id);

int lp_client_pkt_request_label_by_value (struct lib_globals*, struct route_table*,
                                   u_int16_t, struct nsm_client*,
                                   struct lp_client *, u_int8_t,
                                   struct generalized_label*,
                                   enum label_pool_id);

#if defined HAVE_RSVP_GRST || defined HAVE_RESTART

int lp_client_pkt_reserve_label (struct lib_globals *, struct route_table *,
                            u_int16_t, u_int32_t, enum label_pool_id);

int lp_client_pkt_mark_block_stale(struct lib_globals *, struct route_table *,
                              u_int16_t, bool_t, enum label_pool_id);

#endif /* HAVE_RSVP_GRST  || HAVE_RESTART */

#endif /* _PACOS_LP_CLIENT_PACKET_PACKET_H */

