/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_LP_CLIENT_H
#define _PACOS_LP_CLIENT_H

#include "pal.h"
#include "lib.h"
#include "linklist.h"
#include "label_pool.h"
#include "thread.h"
#include "table.h"
#include "nsm_client.h"
#include "bitmap.h"
#include "lp_client.h"

/* This can never be zero !!! */
#define MIN_REQ_FAIL_COUNT             2

/* Bitmap size.  */
#define INDEX_BUCKET_SIZE      640

/* Label set realted */
#define LS_MAX_RANGE_ELEMENTS       2
#define LS_UNBOUND_UPPER_LIMIT      0

/* Macros */
#define REQ_FAIL_QUANTUM(c) \
  ((((c) < MIN_REQ_FAIL_COUNT) ? (c) : MIN_REQ_FAIL_COUNT))
/* End Macros */

/* The next two structs are for processing the received label set operations.
   This struct is used for both the list and the range form of label sets
 */
struct lp_client_glabel_list
{
  /* Count is the number of label values in the array. This will be always
   * two for a range; only the action type will determine as to the
   * applicability of the labels */
  u_int32_t  glabel_count;

  /* array of label can represent:
   * 1. Single element or
   * 2. A range (always 2 elements) or
   * 3. A list (any number of elements)
   */
  struct generalized_label* glabels;
};

/* This struct represents the Label Set Object as defined in RFC 3471 */
struct lp_client_label_set_obj
{
  /* Bit values as defined in nsm_message.h */
  u_int8_t                        action_type;

  struct   lp_client_glabel_list  glabel_list;
};

/* This struct represents a label set as defined in RFC 3471; this may consist
 * of more than one label set objects
 */
struct lp_client_label_set
{
  u_int8_t                        ls_obj_count;
  struct lp_client_label_set_obj  *lbl_set_obj;
};


struct lp_client
{
  /* Label space for this label pool client */
  u_int16_t label_space;

  /* List of label blocks available to the protocol. These set of blocks
   * represent whether a label is not in use (0) or it is already in use (1)
   * Service type index: 0 = RSVP, LDP, BGP; 1 = LDP_PW, 2 = LDP_VPLS  */
  struct bitmap *block_list[LP_SERV_TYPE_MAX];

  /* Back pointer to the route node where this is stored */
  struct route_node *rn;

  /* Count for failed label request queries and associated flag */
#define REQUEST_FAILED                  0
#define RELEASE                         1
  int req_fail_cnt;

  /* Count for releases after atleast one request failure has occured */
  int rel_fail_cnt;

  /* Count of interfaces using this client. */
  u_int32_t refcount;

  /* The clients's service type(s) that have registered to use this instance
   * of lp client */
  enum label_pool_id service_in_use;

};

/* Prototypes */
void lp_client_init (struct lib_globals *, struct route_table **);

u_int32_t lp_client_request_label (struct lib_globals *, struct route_table *,
                              u_int16_t, struct nsm_client *);

int lp_client_release_label (struct lib_globals *, struct route_table *,
                        u_int16_t, u_int32_t, struct nsm_client *);

int lp_client_request_block_send (struct lib_globals *, struct route_table *,
                             u_int16_t, struct nsm_client *);

int lp_client_release_label_cleanup (struct lib_globals *, struct route_table *,
                                u_int16_t, u_int32_t, struct nsm_client *, int);

void lp_client_free (struct lib_globals *, struct route_table *,
                     struct nsm_client *);

u_int32_t  lp_client_get_available_label_count (struct lib_globals *,
                                         struct route_table *, u_int16_t,
                                         u_int32_t);

u_int32_t *lp_client_get_available_labels (struct lib_globals *, struct route_table *,
                                     u_int16_t, u_int32_t *);

int lp_client_check_label_availability (struct lib_globals *, struct route_table *,
                                 u_int16_t);

struct lp_client * lp_client_lookup (struct lib_globals *, struct route_table *,
                                 u_int16_t);

int lp_client_init_label_block (struct lib_globals *, struct route_table *,
                          u_int16_t, struct nsm_client *);

void lp_client_deinit (struct lib_globals *, struct route_table *, u_int16_t,
                   struct nsm_client *);

int lp_client_is_usable (struct lib_globals *, struct route_table *, u_int16_t,
                     u_int32_t);

int lp_client_bind_block (struct lib_globals *, struct route_table *,
                      u_int16_t, u_int32_t, u_int32_t, u_int32_t);

int lp_client_set_service_type(struct lib_globals *, struct route_table *,
                          u_int16_t , enum label_pool_id);

bool_t lp_client_is_service_type_valid(enum label_pool_id);

enum label_pool_id lp_client_get_service_type(module_id_t);

int lp_client_get_service_index (enum label_pool_id);

#if defined HAVE_RSVP_GRST || defined HAVE_RESTART

int lp_client_reserve_label (struct lib_globals *, struct route_table *,
                         u_int16_t, u_int32_t);

#endif /* HAVE_RSVP_GRST  || HAVE_RESTART */

/******************************************************************************
 *
 *                 GMPLS LABEL POOL CLIENT PROTOTYPES
 *
 *****************************************************************************/

struct lp_client* lp_client_gen_new  (struct lib_globals *, struct route_table*,
                                   u_int16_t, struct generalized_label*,
                                   struct generalized_label*,
                                   enum gmpls_label_type, enum label_pool_id);

int lp_client_gen_release_block (struct lib_globals*, struct route_table*,
                            u_int16_t, u_int32_t, enum gmpls_label_type,
                            enum label_pool_id);

int lp_client_gen_release_block_send (struct lib_globals *, struct route_table *,
                                 u_int16_t , u_int32_t , struct nsm_client *nc,
                                 int nsm_cleanup, enum gmpls_label_type,
                                 enum label_pool_id );

void lp_client_gen_init (struct lib_globals *, struct route_table **,
                    enum gmpls_label_type, enum label_pool_id serv_module);

int lp_client_gen_set_service_type (struct lib_globals *, struct route_table *,
                              u_int16_t , enum gmpls_label_type,
                              enum label_pool_id);

struct bitmap_block * lp_client_gen_get_block (struct lib_globals *,
                                           struct route_table *,
                                           u_int16_t, struct generalized_label*,
                                           u_int32_t *, enum gmpls_label_type,
                                           enum label_pool_id);

void lp_client_gen_block_list_free (struct lib_globals *, struct route_table *,
                            struct lp_client *, struct nsm_client *,
                            enum gmpls_label_type, enum label_pool_id);

void lp_client_gen_request_label (struct lib_globals *, struct route_table *,
                             u_int16_t , struct nsm_client *nc,
                             struct generalized_label *glabel,
                             enum gmpls_label_type, enum label_pool_id,
                             u_int32_t, void *);

int lp_client_gen_release_label (struct lib_globals *, struct route_table *,
                            u_int16_t, struct generalized_label*,
                            struct nsm_client *, enum gmpls_label_type  ,
                            enum label_pool_id, u_int32_t, void *);

int lp_client_gen_request_block_send (struct lib_globals *, struct route_table *,
                                 u_int16_t , struct nsm_client *,
                                 enum gmpls_label_type, enum label_pool_id);

int lp_client_gen_release_label_cleanup (struct lib_globals *, struct route_table *,
                                   u_int16_t, struct generalized_label*,
                                   struct nsm_client *, int,
                                   enum gmpls_label_type, enum label_pool_id);

void lp_client_gen_free (struct lib_globals *, struct route_table *,
                     struct nsm_client *, enum gmpls_label_type,
                     enum label_pool_id);

u_int32_t lp_client_gen_get_available_label_count (struct lib_globals *,
                                            struct route_table *, u_int16_t,
                                            u_int32_t , enum gmpls_label_type,
                                            enum label_pool_id);

struct lp_client * lp_client_gen_lookup (struct lib_globals *, struct route_table *,
                                      u_int16_t, enum gmpls_label_type,
                                      enum label_pool_id);

int lp_client_gen_init_label_block (struct lib_globals *, struct route_table *,
                              u_int16_t, struct nsm_client *,
                              enum gmpls_label_type, enum label_pool_id);

void lp_client_gen_deinit (struct lib_globals *, struct route_table *, u_int16_t,
                       struct nsm_client *, enum gmpls_label_type,
                       enum label_pool_id);

int lp_client_gen_is_usable (struct lib_globals *, struct route_table *, u_int16_t ,
                             u_int32_t, struct nsm_client *,
                             enum gmpls_label_type, enum label_pool_id,
                             u_int32_t, void *);

void lp_client_gen_update_request_failure (struct lib_globals *, struct route_table *,
                                    struct lp_client *, u_char,
                                    enum gmpls_label_type, enum label_pool_id);

int lp_client_gen_bind_block (struct lib_globals *, struct route_table *, u_int16_t,
                          u_int32_t, struct generalized_label*,
                          struct generalized_label*, enum gmpls_label_type,
                          enum label_pool_id);

#ifdef HAVE_GMPLS
int lp_client_gen_get_label_from_label_set (struct lib_globals *, struct route_table *,
                                 u_int16_t, struct nsm_client *,
                                 struct lp_client_label_set *,
                                 struct generalized_label *,
                                 enum gmpls_label_type, enum label_pool_id);

int  lp_client_gen_get_label_by_value (struct lib_globals *,
                                 struct route_table *, u_int16_t,
                                 struct nsm_client *, u_int8_t,
                                 struct generalized_label *,
                                 struct lp_client_label_set *,
                                 enum gmpls_label_type, enum label_pool_id);
#endif /*HAVE_GMPLS */

#if defined HAVE_RSVP_GRST || defined HAVE_RESTART

int lp_client_gen_reserve_label (struct lib_globals *, struct route_table *,
                                 u_int16_t, struct generalized_label*,
                                 enum gmpls_label_type, enum label_pool_id);

int lp_client_gen_mark_block_stale (struct lib_globals *, struct route_table *,
                                    u_int16_t, bool_t, enum gmpls_label_type,
                                    enum label_pool_id);

#endif /* HAVE_RSVP_GRST  || HAVE_RESTART */
/* End Prototypes */

#endif /* _PACOS_LP_CLIENT_H */
