/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "prefix.h"
#include "vty.h"
#include "thread.h"
#include "stream.h"
#include "label_pool.h"
#include "nsm_client.h"
#include "network.h"
#include "log.h"
#include "linklist.h"
#include "lp_client.h"
#include "table.h"
#include "nexthop.h"
#include "bitmap.h"
#include "lp_client_packet.h"


/* Create a new label pool client */
struct lp_client *
lp_client_new (struct lib_globals *zg, struct route_table *table,
             u_int16_t label_space, u_int32_t min_label,
             u_int32_t max_label)
{
  struct generalized_label glbl_min;
  struct generalized_label glbl_max;
  enum label_pool_id serv_type = LABEL_POOL_SERVICE_UNSUPPORTED;

  memset(&glbl_min, 0, sizeof (struct generalized_label));
  memset(&glbl_max, 0, sizeof (struct generalized_label));

  if ((serv_type = lp_client_get_service_type(zg->protocol)) ==
                                                      LABEL_POOL_SERVICE_ERROR)
    {
      return NULL;
    }

  glbl_min.gmpls_label_type = GMPLS_LABEL_PACKET;
  glbl_min.u.mpls_label = min_label;

  glbl_max.gmpls_label_type = GMPLS_LABEL_PACKET;
  glbl_max.u.mpls_label = max_label;

  /* Call the generic API with: label format to be that of packet, service
   * type obtained as above and no tlv data */
  return lp_client_gen_new (zg, table, label_space, &glbl_min, &glbl_max,
                            GMPLS_LABEL_PACKET, serv_type);
}


/* Process a bind request's reply.  */
int
lp_client_bind_block (struct lib_globals *zg, struct route_table *table,
                  u_int16_t label_space, u_int32_t block,
                  u_int32_t min_label, u_int32_t max_label)
{
  struct generalized_label glbl_min;
  struct generalized_label glbl_max;
  enum label_pool_id serv_type = LABEL_POOL_SERVICE_ERROR;

  memset(&glbl_min, 0, sizeof (struct generalized_label));
  memset(&glbl_max, 0, sizeof (struct generalized_label));

  serv_type = lp_client_get_service_type(zg->protocol);

  if (serv_type == LABEL_POOL_SERVICE_ERROR)
    {
      return LP_CLIENT_INVALID_RANGE_OWNER;
    }

  glbl_min.gmpls_label_type = GMPLS_LABEL_PACKET;
  glbl_min.u.mpls_label = min_label;

  glbl_max.gmpls_label_type = GMPLS_LABEL_PACKET;
  glbl_max.u.mpls_label = max_label;

  /* Call the generic API with: label format to be that of packet, service
   * type obtained as above and no tlv data */
  return lp_client_gen_bind_block (zg, table, label_space, block, &glbl_min,
                                   &glbl_max, GMPLS_LABEL_PACKET, serv_type);
}

/* Process a release request's reply. */
int
lp_client_release_block (struct lib_globals *zg, struct route_table *table,
                         u_int16_t label_space, u_int32_t block)
{
  enum label_pool_id serv_type = LABEL_POOL_SERVICE_ERROR;

  serv_type = lp_client_get_service_type(zg->protocol);

  if (serv_type == LABEL_POOL_SERVICE_ERROR)
    {
      return LP_CLIENT_INVALID_RANGE_OWNER;
    }

  return lp_client_gen_release_block (zg, table, label_space, block,
                                       GMPLS_LABEL_PACKET, serv_type);
}

/* Send label block request message to NSM. */
int
lp_client_request_block_send (struct lib_globals *zg,
                         struct route_table *table,
                         u_int16_t label_space,
                         struct nsm_client *nc)
{
  enum label_pool_id serv_type = LABEL_POOL_SERVICE_ERROR;

  serv_type = lp_client_get_service_type(zg->protocol);

  if (serv_type == LABEL_POOL_SERVICE_ERROR)
    {
      return LP_CLIENT_INVALID_RANGE_OWNER;
    }

  return lp_client_gen_request_block_send (zg, table, label_space, nc,
                                                GMPLS_LABEL_PACKET, serv_type);
}

/* Send label block release message to NSM. */
int
lp_client_release_block_send (struct lib_globals *zg,
                              struct route_table *table,
                              u_int16_t label_space,
                              u_int32_t block,
                              struct nsm_client *nc,
                              int nsm_cleanup)
{
  enum label_pool_id serv_type = LABEL_POOL_SERVICE_ERROR;

  serv_type = lp_client_get_service_type(zg->protocol);

  if (serv_type == LABEL_POOL_SERVICE_ERROR)
    {
      return LP_CLIENT_INVALID_RANGE_OWNER;
    }

  return lp_client_gen_release_block_send (zg, table, label_space, block, nc,
                                  nsm_cleanup, GMPLS_LABEL_PACKET, serv_type);
}

/* Get the label pool client specified. If it's not in the table,
   create it.  */
struct lp_client *
lp_client_lookup (struct lib_globals *zg, struct route_table *table,
                  u_int16_t label_space)
{
  enum label_pool_id serv_type = LABEL_POOL_SERVICE_ERROR;

  serv_type = lp_client_get_service_type(zg->protocol);

  if (serv_type == LABEL_POOL_SERVICE_ERROR)
    {
      return NULL;
    }

  return (lp_client_gen_lookup (zg, table, label_space,
                                GMPLS_LABEL_PACKET, serv_type));
}

/* API to be used by protocols.  Request the first block during
   initialization of an interface on the protocol end.  */
int
lp_client_init_label_block (struct lib_globals *zg,
                            struct route_table *table,
                            u_int16_t label_space,
                            struct nsm_client *nc)
{
  enum label_pool_id serv_type = LABEL_POOL_SERVICE_ERROR;

  serv_type = lp_client_get_service_type(zg->protocol);

  if (serv_type == LABEL_POOL_SERVICE_ERROR)
    {
      return LP_CLIENT_INVALID_RANGE_OWNER;
    }

  return (lp_client_gen_init_label_block (zg, table, label_space, nc,
                                       GMPLS_LABEL_PACKET, serv_type));
}

void
lp_client_init (struct lib_globals *zg, struct route_table **table)
{
  enum label_pool_id serv_type = LABEL_POOL_SERVICE_ERROR;

  serv_type = lp_client_get_service_type(zg->protocol);

  if (serv_type == LABEL_POOL_SERVICE_ERROR)
    {
      /* No error value; undefined value for table indicates an error case */
      return;
    }

  lp_client_gen_init (zg, table, GMPLS_LABEL_PACKET, serv_type);
}

int
lp_client_set_service_type(struct lib_globals *zg,
                      struct route_table *table,
                      u_int16_t label_space,
                      enum label_pool_id serv_module)
{
  enum label_pool_id serv_type = LABEL_POOL_SERVICE_ERROR;

  serv_type = lp_client_get_service_type(zg->protocol);

  if (serv_type == LABEL_POOL_SERVICE_ERROR)
    {
      return LP_CLIENT_INVALID_RANGE_OWNER;
    }

  return lp_client_gen_set_service_type (zg, table, label_space,
                                          GMPLS_LABEL_PACKET, serv_type);
}

void
lp_client_block_list_free (struct lib_globals *zg,
                    struct route_table *table,
                    struct lp_client *client,
                    struct nsm_client *nc)
{
  enum label_pool_id serv_type = LABEL_POOL_SERVICE_ERROR;

  serv_type = lp_client_get_service_type(zg->protocol);

  if (serv_type == LABEL_POOL_SERVICE_ERROR)
    {
      return;
    }

  lp_client_gen_block_list_free (zg, table, client, nc, GMPLS_LABEL_PACKET,
                                 serv_type);
}

/* Free label pool.  */
void
lp_client_free (struct lib_globals *zg,
            struct route_table *table,
            struct nsm_client *nc)
{
  enum label_pool_id serv_type = LABEL_POOL_SERVICE_ERROR;

  serv_type = lp_client_get_service_type(zg->protocol);

  if (serv_type == LABEL_POOL_SERVICE_ERROR)
    {
      return;
    }

  lp_client_gen_free (zg, table, nc, GMPLS_LABEL_PACKET, serv_type);
}

/* Get the total number of labels in the list, excluding the block
   corresponding to the block id passed in.  */
u_int32_t
lp_client_get_available_label_count (struct lib_globals *zg,
                              struct route_table *table,
                              u_int16_t label_space,
                              u_int32_t block_id)
{
  enum label_pool_id serv_type = LABEL_POOL_SERVICE_ERROR;

  serv_type = lp_client_get_service_type(zg->protocol);

  if (serv_type == LABEL_POOL_SERVICE_ERROR)
    {
      return LP_CLIENT_INVALID_RANGE_OWNER;
    }

  return lp_client_gen_get_available_label_count  (zg, table, label_space,
                                       block_id, GMPLS_LABEL_PACKET, serv_type);
}

/* Check if there is atleast one label available. */
int
lp_client_is_usable (struct lib_globals *zg,
                struct route_table *table,
                u_int16_t label_space,
                u_int32_t block_id)
{
  enum label_pool_id serv_type = LABEL_POOL_SERVICE_ERROR;

  serv_type = lp_client_get_service_type(zg->protocol);

  if (serv_type == LABEL_POOL_SERVICE_ERROR)
    {
      return LP_CLIENT_INVALID_RANGE_OWNER;
    }

  /* Passing the argument for nsm_client as NULL */
  return lp_client_gen_is_usable (zg, table, label_space, block_id,
                                       NULL, GMPLS_LABEL_PACKET, serv_type, 0, NULL);
}

/* Helper function to update the request failure counter.  */
void
lp_client_update_request_failure (struct lib_globals *zg,
                                  struct route_table *table,
                                  struct lp_client *client,
                                  u_char type)
{
  enum label_pool_id serv_type = LABEL_POOL_SERVICE_ERROR;

  serv_type = lp_client_get_service_type(zg->protocol);

  if (serv_type == LABEL_POOL_SERVICE_ERROR)
    {
      return;
    }

  return lp_client_gen_update_request_failure (zg, table, client, type,
                                                GMPLS_LABEL_PACKET, serv_type);
}

struct bitmap_block *
lp_client_get_block (struct lib_globals *zg, struct route_table *table,
                     u_int16_t label_space, u_int32_t label,
                     u_int32_t *req_fail_cnt)
{
  struct generalized_label gen_lbl;
  enum label_pool_id serv_type = LABEL_POOL_SERVICE_ERROR;

  serv_type = lp_client_get_service_type(zg->protocol);

  if (serv_type == LABEL_POOL_SERVICE_ERROR)
    {
      return NULL;
    }

  memset(&gen_lbl, 0, sizeof (struct generalized_label));
  gen_lbl.gmpls_label_type = GMPLS_LABEL_PACKET;
  gen_lbl.u.mpls_label = label;

  return lp_client_gen_get_block (zg, table, label_space, &gen_lbl,
                                  req_fail_cnt, GMPLS_LABEL_PACKET, serv_type);
}

#if defined HAVE_RSVP_GRST || defined HAVE_RESTART
int
lp_client_reserve_label (struct lib_globals *zg,
                    struct route_table *table,
                    u_int16_t label_space,
                    u_int32_t label)
{
  struct generalized_label gen_lbl;
  enum label_pool_id serv_type = LABEL_POOL_SERVICE_ERROR;

  serv_type = lp_client_get_service_type(zg->protocol);

  if (serv_type == LABEL_POOL_SERVICE_ERROR)
    {
      return LP_CLIENT_INVALID_RANGE_OWNER;
    }

  memset(&gen_lbl, 0, sizeof (struct generalized_label));
  gen_lbl.gmpls_label_type = GMPLS_LABEL_PACKET;
  gen_lbl.u.mpls_label = label;

  return lp_client_gen_reserve_label (zg, table, label_space, &gen_lbl,
                                       GMPLS_LABEL_PACKET, serv_type);
}
#endif  /* HAVE_RSVP_GRST  || HAVE_RESTART */

u_int32_t
lp_client_request_label (struct lib_globals *zg,
                    struct route_table *table,
                    u_int16_t label_space,
                    struct nsm_client *nc)
{
  struct generalized_label gen_lbl;
  enum label_pool_id serv_type = LABEL_POOL_SERVICE_ERROR;

  serv_type = lp_client_get_service_type(zg->protocol);

  if (serv_type == LABEL_POOL_SERVICE_ERROR)
    {
      return LP_CLIENT_INVALID_RANGE_OWNER;
    }

  memset(&gen_lbl, 0, sizeof (struct generalized_label));
  gen_lbl.gmpls_label_type = GMPLS_LABEL_PACKET;

  lp_client_gen_request_label(zg, table, label_space, nc, &gen_lbl,
                              GMPLS_LABEL_PACKET, serv_type, LP_DATA_LEN_ZERO,
                              LP_DATA_NULL);

  return (gen_lbl.u.mpls_label);
}

int
lp_client_release_label (struct lib_globals *zg,
                     struct route_table *table,
                     u_int16_t label_space,
                     u_int32_t label,
                     struct nsm_client *nc)
{
  struct generalized_label gen_lbl;
  enum label_pool_id serv_type = LABEL_POOL_SERVICE_ERROR;

  serv_type = lp_client_get_service_type(zg->protocol);

  if (serv_type == LABEL_POOL_SERVICE_ERROR)
    {
      return LP_CLIENT_INVALID_RANGE_OWNER;
    }

  memset(&gen_lbl, 0, sizeof (struct generalized_label));
  gen_lbl.gmpls_label_type = GMPLS_LABEL_PACKET;
  gen_lbl.u.mpls_label = label;

  return lp_client_gen_release_label (zg, table, label_space, &gen_lbl, nc,
                                       GMPLS_LABEL_PACKET, serv_type,
                                       LP_DATA_LEN_ZERO, LP_DATA_NULL);
}

int
lp_client_release_label_cleanup (struct lib_globals *zg,
                            struct route_table *table,
                            u_int16_t label_space,
                            u_int32_t label,
                            struct nsm_client *nc,
                            int nsm_cleanup)
{
  struct generalized_label gen_lbl;
  enum label_pool_id serv_type = LABEL_POOL_SERVICE_ERROR;

  serv_type = lp_client_get_service_type(zg->protocol);

  if (serv_type == LABEL_POOL_SERVICE_ERROR)
    {
      return LP_CLIENT_INVALID_RANGE_OWNER;
    }

  memset(&gen_lbl, 0, sizeof (struct generalized_label));
  gen_lbl.gmpls_label_type = GMPLS_LABEL_PACKET;
  gen_lbl.u.mpls_label = label;

  return lp_client_gen_release_label_cleanup (zg, table, label_space, &gen_lbl,
                                nc, nsm_cleanup, GMPLS_LABEL_PACKET, serv_type);
}

bool_t
lp_client_is_service_type_valid(enum label_pool_id serv_type)
{
  if ((serv_type == LABEL_POOL_SERVICE_RSVP) ||
      (serv_type == LABEL_POOL_SERVICE_LDP)  ||
      (serv_type == LABEL_POOL_SERVICE_BGP))
    {
      return PAL_TRUE;
    }
  return PAL_FALSE;
}

/* This internal function returns the service type of a protocol (module)
 * provided as argument */
enum label_pool_id
lp_client_get_service_type(module_id_t protocol)
{
  switch (protocol)
    {
      case APN_PROTO_RSVP:
        return LABEL_POOL_SERVICE_RSVP;

      case APN_PROTO_LDP:
        return LABEL_POOL_SERVICE_LDP;

      case APN_PROTO_BGP:
        return LABEL_POOL_SERVICE_BGP;

      default:
        return LABEL_POOL_SERVICE_ERROR;
    }
}

int
lp_client_get_service_index (enum label_pool_id serv_type)
{
  switch (serv_type)
    {
      case LABEL_POOL_SERVICE_RSVP:
      case LABEL_POOL_SERVICE_LDP:
      case LABEL_POOL_SERVICE_BGP:
        return 0;

      case LABEL_POOL_SERVICE_LDP_LDP_PW:
        return 1;

      case LABEL_POOL_SERVICE_LDP_VPLS:
        return 2;

      default:
        /* Should not happen as the enum coming in is already validated */
        return LABEL_POOL_SERVICE_ERROR;
    }
}

/* Release label pool client. */
void
lp_client_deinit (struct lib_globals *zg, struct route_table *table,
                  u_int16_t label_space, struct nsm_client *nc)
{
  enum label_pool_id serv_type = LABEL_POOL_SERVICE_ERROR;

  serv_type = lp_client_get_service_type(zg->protocol);

  if (serv_type == LABEL_POOL_SERVICE_ERROR)
    {
      /* Need a critical error or alarm here */
      return;
    }

  lp_client_gen_deinit (zg, table, label_space, nc, GMPLS_LABEL_PACKET,
                                serv_type);
}

/*******************************************************************************
 *******************************************************************************
 *
 *                 GMPLS LABEL POOL CLIENT SECTION
 *
 *******************************************************************************
 ******************************************************************************/
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE

void
lp_client_request_pbb_te_label  (struct lib_globals *zg,
                         struct route_table *table,
                         u_int16_t label_space,
                         struct nsm_client *nc,
                         struct generalized_label *glabel,
                         enum gmpls_label_type lbl_format,
                         enum label_pool_id serv_module,
                         u_int32_t tlv_data_len,
                         void *tlv_data)
{
  int ret = 0;
  struct nsm_msg_generic_label_pool msg;
  struct nsm_msg_generic_label_pool reply;
  struct nsm_glabel_pbb_te_service_data pbb_te_data_send;
  struct nsm_glabel_pbb_te_service_data pbb_te_data_recv;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_generic_label_pool));
  pal_mem_set (&reply, 0, sizeof (struct nsm_msg_generic_label_pool));
  pal_mem_set (&pbb_te_data_send, 0,
               sizeof (struct nsm_glabel_pbb_te_service_data));
  pal_mem_set (&pbb_te_data_recv, 0,
               sizeof (struct nsm_glabel_pbb_te_service_data));

  /* Setting the data space for the tech specific data */
  msg.u.pb_te_service_data = &pbb_te_data_send;
  reply.u.pb_te_service_data = &pbb_te_data_recv;

  /* Now request for the label */
  NSM_SET_CTYPE (msg.cindex, NSM_LABEL_POOL_CTYPE_PBB_TE_LABEL);
  msg.label_space = label_space;

  if(tlv_data)
    pal_mem_cpy(msg.u.pb_te_service_data,
                (struct nsm_glabel_pbb_te_service_data *)tlv_data,
                sizeof(struct nsm_glabel_pbb_te_service_data));
  else
    return;

  ret = nsm_client_send_generic_label_msg (nc,
                  NSM_MSG_GENERIC_LABEL_POOL_REQUEST, &msg, &reply);
  if (ret < 0)
    return;

  /* Copying the reply pbb structure to the tlv data
     Will be used later for the tesid and result */
  pal_mem_cpy((struct nsm_glabel_pbb_te_service_data *)tlv_data,
               reply.u.pb_te_service_data,
               sizeof(struct nsm_glabel_pbb_te_service_data));

  /* Update the label and send back */
  if((reply.u.pb_te_service_data)->result == LP_CLIENT_SUCCESS)
  {
    glabel->gmpls_label_type = lbl_format;
    pal_mem_cpy(&glabel->u.pbb_label,
                         &(reply.u.pb_te_service_data)->pbb_label,
                         sizeof(struct pbb_te_label));
  }
  else
    glabel->gmpls_label_type = GMPLS_LABEL_ERROR;

  return;
}

int
lp_client_release_pbb_te_label(struct lib_globals *zg,
                            struct route_table *table,
                            u_int16_t label_space,
                            struct generalized_label* glabel,
                            struct nsm_client *nc,
                            enum gmpls_label_type lbl_format,
                            enum label_pool_id serv_module,
                            u_int32_t tlv_data_len,
                            void *tlv_data)
{
  int ret = 0;
  struct nsm_msg_generic_label_pool msg;
  struct nsm_msg_generic_label_pool reply;
  struct nsm_glabel_pbb_te_service_data pbb_te_data_send;
  struct nsm_glabel_pbb_te_service_data pbb_te_data_recv;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_generic_label_pool));
  pal_mem_set (&reply, 0, sizeof (struct nsm_msg_generic_label_pool));
  pal_mem_set (&pbb_te_data_send, 0,
               sizeof (struct nsm_glabel_pbb_te_service_data));
  pal_mem_set (&pbb_te_data_recv, 0,
               sizeof (struct nsm_glabel_pbb_te_service_data));

  /* Setting the data space for the tech specific data */
  msg.u.pb_te_service_data = &pbb_te_data_send;
  reply.u.pb_te_service_data = &pbb_te_data_recv;

  /* Now request for the label */
  NSM_SET_CTYPE (msg.cindex, NSM_LABEL_POOL_CTYPE_PBB_TE_LABEL);
  msg.label_space = label_space;

  if(tlv_data)
    pal_mem_cpy(msg.u.pb_te_service_data,
                (struct nsm_glabel_pbb_te_service_data *)tlv_data,
                sizeof(struct nsm_glabel_pbb_te_service_data));
  else
    return LP_CLIENT_FAILURE;

  ret = nsm_client_send_generic_label_msg (nc,
                  NSM_MSG_GENERIC_LABEL_POOL_RELEASE, &msg, &reply);
  if (ret < 0)
    return LP_CLIENT_FAILURE;

  /* Update the label and send back */
  return (reply.u.pb_te_service_data)->result;
}

int
lp_client_pbb_te_usable(struct lib_globals *zg,
                    struct route_table *table,
                    u_int16_t label_space,
                    u_int32_t block_id,
                    struct nsm_client *nc,
                    enum gmpls_label_type lbl_format,
                    enum label_pool_id serv_module,
                    u_int32_t tlv_data_len,
                    void *tlv_data)
{
  int ret = 0;
  struct nsm_msg_generic_label_pool msg;
  struct nsm_msg_generic_label_pool reply;
  struct nsm_glabel_pbb_te_service_data pbb_te_data_send;
  struct nsm_glabel_pbb_te_service_data pbb_te_data_recv;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_generic_label_pool));
  pal_mem_set (&reply, 0, sizeof (struct nsm_msg_generic_label_pool));
  pal_mem_set (&pbb_te_data_send, 0,
               sizeof (struct nsm_glabel_pbb_te_service_data));
  pal_mem_set (&pbb_te_data_recv, 0,
               sizeof (struct nsm_glabel_pbb_te_service_data));

  /* Setting the data space for the tech specific data */
  msg.u.pb_te_service_data = &pbb_te_data_send;
  reply.u.pb_te_service_data = &pbb_te_data_recv;

  /* Now request for the label */
  NSM_SET_CTYPE (msg.cindex, NSM_LABEL_POOL_CTYPE_PBB_TE_LABEL);
  msg.label_space = label_space;

  if(tlv_data)
    pal_mem_cpy(msg.u.pb_te_service_data,
                (struct nsm_glabel_pbb_te_service_data *)tlv_data,
                sizeof(struct nsm_glabel_pbb_te_service_data));
  else
    return LP_CLIENT_FAILURE;

  ret = nsm_client_send_generic_label_msg (nc,
                  NSM_MSG_GENERIC_LABEL_POOL_IN_USE, &msg, &reply);
  if (ret < 0)
    return LP_CLIENT_FAILURE;

  /* Update the label and send back */
  return (reply.u.pb_te_service_data)->result;
}

#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */

struct lp_client*
lp_client_pkt_new_wrap(struct lib_globals *zg,
                        struct route_table *table,
                        u_int16_t label_space,
                        struct generalized_label* gmin_label,
                        struct generalized_label* gmax_label,
                        enum gmpls_label_type lbl_format,
                        enum label_pool_id serv_module)
{
  return (lp_client_pkt_new (zg, table, label_space,
                       gmin_label->u.mpls_label,
                       gmax_label->u.mpls_label,
                       serv_module));
}

/*
* Function table initilaization to invoke the APIs corresponding to different
* packet technologies as defined in enum gmpls_label_type .
* The funcation table array element must be NULL if a particular label type is
* not supported or the label type doesn't support the respective service, in
* this case requesting a new instance of label pool client.
*/
static struct lp_client* (* const gen_lpc_new_pf[])
                                   (struct lib_globals *zg,
                                    struct route_table *table,
                                    u_int16_t label_space,
                                    struct generalized_label* gmin_label,
                                    struct generalized_label* gmax_label,
                                    enum gmpls_label_type  lbl_format,
                                    enum label_pool_id serv_module) =
  {
#ifdef HAVE_PACKET
  lp_client_pkt_new_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  };

/** @brief Initiates the creation of technology specific lp client instance

    @param zg - daemon specific library global
    @param table - table holding the lp client instances
    @param label_space - label space id
    @param gmin_lbl - minimum value of the labe space
    @param gmax_lbl - maximim value of the labe space
    @param lbl_format - type label technology
    @param serv_module - client or module type

    @return - new lp client instance if successful; NULL otherwise
*/
struct lp_client*
lp_client_gen_new  (struct lib_globals *zg,
                 struct route_table *table,
                 u_int16_t label_space,
                 struct generalized_label* gmin_lbl,
                 struct generalized_label* gmax_lbl,
                 enum gmpls_label_type  lbl_format,
                 enum label_pool_id serv_module)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lpc_new_pf[lbl_format]) (zg, table, label_space, gmin_lbl,
                                             gmax_lbl, lbl_format, serv_module);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS */
      default:
        return NULL;
        break;
    }
}

int
lp_client_pkt_release_block_wrap(struct lib_globals *zg,
                        struct route_table *table,
                        u_int16_t label_space,
                        u_int32_t block_id,
                        enum gmpls_label_type  lbl_format,
                        enum label_pool_id serv_module)
{
  return (lp_client_pkt_release_block (zg, table, label_space, block_id,
                                          serv_module));
}

/*
* Function table initilaization to invoke the APIs corresponding to different
* packet technologies as defined in enum gmpls_label_type .
* The funcation table array element must be NULL if a particular label type is
* not supported or the label type doesn't support the respective service, in
* this case release a label pool block.
*/
static int (* const gen_lpc_rel_blk_pf[]) (struct lib_globals *zg,
                                   struct route_table *table,
                                   u_int16_t label_space,
                                   u_int32_t block_id,
                                   enum gmpls_label_type  lbl_format,
                                   enum label_pool_id serv_module) =
  {
#ifdef HAVE_PACKET
  lp_client_pkt_release_block_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  };

/** @brief Initiates the deletion of a block of labels

    @param zg - daemon specific library global
    @param table - table holding the lp client instances
    @param label_space - label space id
    @param block_id - block id to be deleted
    @param lbl_format - type label technology
    @param serv_module - client or module type

    @return - LP_CLIENT_SUCCESS or LP_CLIENT_FAILURE
*/
int
lp_client_gen_release_block (struct lib_globals *zg,
                         struct route_table *table,
                         u_int16_t label_space,
                         u_int32_t block_id,
                         enum gmpls_label_type  lbl_format,
                         enum label_pool_id lp_module)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lpc_rel_blk_pf[lbl_format]) (zg, table, label_space,
                                             block_id, lbl_format, lp_module);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS */
      default:
        return LP_CLIENT_INVALID_LABEL_TYPE;
        break;
    }
}

int
lp_client_pkt_release_block_send_wrap(struct lib_globals *zg,
                                 struct route_table *table,
                                 u_int16_t label_space,
                                 u_int32_t block_id,
                                 struct nsm_client *nc,
                                 int nsm_cleanup,
                                 enum gmpls_label_type  lbl_format,
                                 enum label_pool_id serv_module)
{
  return (lp_client_pkt_release_block_send (zg, table, label_space,
                                    block_id, nc, nsm_cleanup, serv_module));
}

/*
* Function table initilaization to invoke the APIs corresponding to different
* packet technologies as defined in enum gmpls_label_type .
* The funcation table array element must be NULL if a particular label type is
* not supported or the label type doesn't support the respective service, in
* this case dispatching a message to release a label pool block.
*/
static int (* const gen_lpc_rel_blktx_pf[]) (struct lib_globals *zg,
                                         struct route_table *table,
                                         u_int16_t label_space,
                                         u_int32_t block_id,
                                         struct nsm_client *nc,
                                         int nsm_cleanup,
                                         enum gmpls_label_type  lbl_format,
                                         enum label_pool_id serv_module) =
  {
#ifdef HAVE_PACKET
  lp_client_pkt_release_block_send_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  };

/** @brief Sends a block deletion message to NSM if nsm_cleanup is true and
           cleans up lp client instance as well

    @param zg - daemon specific library global
    @param table - table holding the lp client instances
    @param label_space - label space id
    @param block_id - block id to be deleted
    @param nc - lp client instance
    @param nsm_cleanup - if TRUE, NSM message is dispatched
    @param lbl_format - type label technology
    @param serv_module - client or module type

    @return - LP_CLIENT_SUCCESS or LP_CLIENT_FAILURE
*/
int
lp_client_gen_release_block_send (struct lib_globals *zg,
                             struct route_table *table,
                             u_int16_t label_space,
                             u_int32_t block_id,
                             struct nsm_client *nc,
                             int nsm_cleanup,
                             enum gmpls_label_type  lbl_format,
                             enum label_pool_id lp_module)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lpc_rel_blktx_pf[lbl_format]) (zg, table, label_space,
                                                    block_id, nc, nsm_cleanup,
                                                    lbl_format, lp_module);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS */
      default:
        return LP_CLIENT_INVALID_LABEL_TYPE;
        break;
    }
}

void
lp_client_pkt_init_wrap (struct lib_globals *zg,
                    struct route_table **table,
                    enum gmpls_label_type  lbl_format,
                    enum label_pool_id serv_module)
{
  return (lp_client_pkt_init (zg, table));
}

/*
* Function table initilaization to invoke the APIs corresponding to different
* packet technologies as defined in enum gmpls_label_type .
* The funcation table array element must be NULL if a particular label type is
* not supported or the label type doesn't support the respective service, in
* this case add an instance to top level labe pool table.
*/
static void (* const gen_lpc_init_pf[]) (struct lib_globals *zg,
                                        struct route_table **table,
                                        enum gmpls_label_type lbl_format,
                                        enum label_pool_id serv_module) =
  {
#ifdef HAVE_PACKET
  lp_client_pkt_init_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  };

/** @brief Sets up technology specific lp client table to hold lp struct
           instances

    @param zg - daemon specific library global
    @param table - table holding the lp client instances
    @param label_space - label space id
    @param block_id - block id to be deleted
    @param nc - lp client instance
    @param nsm_cleanup - if TRUE, NSM message is dispatched
    @param lbl_format - type label technology
    @param serv_module - client or module type

    @return - LP_CLIENT_SUCCESS or LP_CLIENT_FAILURE
*/
void
lp_client_gen_init (struct lib_globals *zg,
                struct route_table **table,
                enum gmpls_label_type  lbl_format,
                enum label_pool_id serv_module)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lpc_init_pf[lbl_format]) (zg, table, lbl_format,
                                               serv_module);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS */
      default:
        return;
        break;
    }
}

int
lp_client_pkt_set_service_type_wrap (struct lib_globals *zg,
                              struct route_table *table,
                              u_int16_t label_space,
                              enum gmpls_label_type  lbl_format,
                              enum label_pool_id serv_module)
{
  return (lp_client_pkt_set_service_type (zg, table, label_space, serv_module));
}

/*
* Function table initilaization to invoke the APIs corresponding to different
* packet technologies as defined in enum gmpls_label_type .
* The funcation table array element must be NULL if a particular label type is
* not supported or the label type doesn't support the respective service, in
* this case send label block request message to NSM.
*/
static int (* const gen_lpc_sst_pf[]) (struct lib_globals *zg,
                                       struct route_table *table,
                                       u_int16_t label_space,
                                       enum gmpls_label_type  lbl_format,
                                       enum label_pool_id serv_module) =
  {
#ifdef HAVE_PACKET
  lp_client_pkt_set_service_type_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  };

/** @brief Sets the service type of the module that owns the current lp client

    @param zg - daemon specific library global
    @param table - table holding the lp client instances
    @param label_space - label space id
    @param lbl_format - type label technology
    @param serv_module - client or module type

    @return - LP_CLIENT_SUCCESS or or an erro code
*/
int
lp_client_gen_set_service_type (struct lib_globals *zg,
                          struct route_table *table,
                          u_int16_t label_space,
                          enum gmpls_label_type  lbl_format,
                          enum label_pool_id serv_module)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lpc_sst_pf[lbl_format]) (zg, table, label_space,
                                              lbl_format, serv_module);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS */
      default:
        return LP_CLIENT_INVALID_LABEL_TYPE;
        break;
    }
}

struct bitmap_block *
lp_client_pkt_get_block_wrap (struct lib_globals *zg,
                         struct route_table *table,
                         u_int16_t label_space,
                         struct generalized_label *glbl,
                         u_int32_t *req_fail_cnt,
                         enum gmpls_label_type  lbl_format,
                         enum label_pool_id serv_module)
{
  return (lp_client_pkt_get_block (zg, table, label_space, glbl->u.mpls_label,
                                   req_fail_cnt, serv_module));
}

/*
* Function table initilaization to invoke the APIs corresponding to different
* packet technologies as defined in enum gmpls_label_type .
* The funcation table array element must be NULL if a particular label type is
* not supported or the label type doesn't support the respective service, in
* this case send label block request message to NSM.
*/
static struct bitmap_block* (* const gen_lpc_gblk_pf[])
                                       (struct lib_globals *zg,
                                        struct route_table *table,
                                        u_int16_t label_space,
                                        struct generalized_label *glbl,
                                        u_int32_t *req_fail_cnt,
                                        enum gmpls_label_type  lbl_format,
                                        enum label_pool_id serv_module) =
{
#ifdef HAVE_PACKET
  lp_client_pkt_get_block_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  };

/** @brief This API provides the block corresponding to the label specified if
           it exists within the block list for the lp client and service module

    @param zg - daemon specific library global
    @param table - table holding the lp client instances
    @param label_space - label space id
    @param glbl - label for which the block is requested
    @param req_fail_cnt - failure counter
    @param lbl_format - type label technology
    @param serv_module - client or module type

    @return - Success: pointer to bitmap block or NULL otherwise
*/
struct bitmap_block *
lp_client_gen_get_block (struct lib_globals *zg,
                     struct route_table *table,
                     u_int16_t label_space,
                     struct generalized_label *glbl,
                     u_int32_t *req_fail_cnt,
                     enum gmpls_label_type  lbl_format,
                     enum label_pool_id serv_module)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lpc_gblk_pf[lbl_format]) (zg, table, label_space, glbl,
                                      req_fail_cnt, lbl_format, serv_module);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS */
      default:
        return NULL;
        break;
    }
}

void
lp_client_pkt_block_list_free_wrap (struct lib_globals *zg,
                             struct route_table *table,
                             struct lp_client *client,
                             struct nsm_client *nc,
                             enum gmpls_label_type  lbl_format,
                             enum label_pool_id serv_module)
{
  return (lp_client_pkt_block_list_free (zg, table, client, nc, serv_module));
}

/*
* Function table initilaization to invoke the APIs corresponding to different
* packet technologies as defined in enum gmpls_label_type .
* The funcation table array element must be NULL if a particular label type is
* not supported or the label type doesn't support the respective service, in
* this case send label block request message to NSM.
*/
static void (* const gen_lpc_blklfree_pf[]) (struct lib_globals *zg,
                                            struct route_table *table,
                                            struct lp_client *client,
                                            struct nsm_client *nc,
                                            enum gmpls_label_type lbl_format,
                                            enum label_pool_id serv_module) =
  {
#ifdef HAVE_PACKET
  lp_client_pkt_block_list_free_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  };

/** @brief Deletes all the blocks in the block list and the block list itself

    @param zg - daemon specific library global
    @param table - table holding the lp client instances
    @param client - lp client instance
    @param nc - nsm client instance
    @param lbl_format - type label technology
    @param serv_module - client or module type

    @return - none
*/
void
lp_client_gen_block_list_free (struct lib_globals *zg,
                         struct route_table *table,
                         struct lp_client *client,
                         struct nsm_client *nc,
                         enum gmpls_label_type  lbl_format,
                         enum label_pool_id serv_module)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lpc_blklfree_pf[lbl_format]) (zg, table, client, nc,
                                                    lbl_format, serv_module);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS */
      default:
        return;
        break;
    }
}

void
lp_client_pkt_request_label_wrap(struct lib_globals *zg,
                            struct route_table *table,
                            u_int16_t label_space,
                            struct nsm_client *nc,
                            struct generalized_label *glabel,
                            enum gmpls_label_type lbl_format,
                            enum label_pool_id serv_module,
                            u_int32_t tlv_data_len,
                            void *tlv_data)
{
  glabel->u.mpls_label = lp_client_pkt_request_label (zg, table,
                                                 label_space, nc, serv_module);
  if (glabel->u.mpls_label == LABEL_VALUE_INVALID)
    {
      glabel->gmpls_label_type = GMPLS_LABEL_ERROR;
    }
}

/*
* Function table initilaization to invoke the APIs corresponding to different
* packet technologies as defined in enum gmpls_label_type .
* The funcation table array element must be NULL if a particular label type is
* not supported or the label type doesn't support the respective service, in
* this case requesting a generic label from the label pool cleint or server.
*/
static void (* const gen_lpc_req_lbl_pf[])
                                   (struct lib_globals *zg,
                                    struct route_table *table,
                                    u_int16_t label_space,
                                    struct nsm_client *nc,
                                    struct generalized_label *glabel,
                                    enum gmpls_label_type  lbl_format,
                                    enum label_pool_id serv_module,
                                    u_int32_t tlv_data_len,
                                    void *tlv_data) =
{
#ifdef HAVE_PACKET
  lp_client_pkt_request_label_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  lp_client_request_pbb_te_label,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
};

/** @brief This API can be used to request a label for the given label space
           for a psecified service module

    @param zg - daemon specific library global
    @param table - table holding the lp client instances
    @param label_space - label space id
    @param block_id - block id to be deleted
    @param nc - lp client instance
    @param nsm_cleanup - if TRUE, NSM message is dispatched
    @param lbl_format - type label technology
    @param serv_module - client or module type

    @return - Success: a valid gmpls label value
              Failure: label shall be LABEL_VALUE_INVALID for pkt and
                       the gmpls albel type will be GMPLS_LABEL_ERROR
*/
void
lp_client_gen_request_label  (struct lib_globals *zg,
                         struct route_table *table,
                         u_int16_t label_space,
                         struct nsm_client *nc,
                         struct generalized_label *glabel,
                         enum gmpls_label_type  lbl_format,
                         enum label_pool_id serv_module,
                         u_int32_t tlv_data_len,
                         void *tlv_data)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case GMPLS_LABEL_PBB_TE:
#endif /* HAVE_PB_TE */
#endif /* HAVE_GMPLS */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lpc_req_lbl_pf[lbl_format]) (zg, table, label_space, nc,
                      glabel, lbl_format, serv_module, tlv_data_len, tlv_data);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS */
      default:
        return;
        break;
    }
}

int
lp_client_pkt_release_label_wrap(struct lib_globals *zg,
                            struct route_table *table,
                            u_int16_t label_space,
                            struct generalized_label* glabel,
                            struct nsm_client *nc,
                            enum gmpls_label_type  lbl_format,
                            enum label_pool_id serv_module,
                            u_int32_t tlv_data_len,
                            void *tlv_data)
{
  return (lp_client_pkt_release_label (zg, table, label_space,
                            glabel->u.mpls_label, nc, serv_module));
}

/*
* Function table initilaization to invoke the APIs corresponding to different
* packet technologies as defined in enum gmpls_label_type .
* The funcation table array element must be NULL if a particular label type is
* not supported or the label type doesn't support the respective service, in
* this case releasing a generic label from the label pool client by invoking
* lp_client_release_label_cleanup.
*/
static int (* const gen_lpc_rel_lbl_pf[]) (struct lib_globals *zg,
                                           struct route_table *table,
                                           u_int16_t label_space,
                                           struct generalized_label* glabel,
                                           struct nsm_client *nc,
                                           enum gmpls_label_type  lbl_format,
                                           enum label_pool_id serv_module,
                                           u_int32_t tlv_data_len,
                                           void *tlv_data) =
{
#ifdef HAVE_PACKET
  lp_client_pkt_release_label_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  lp_client_release_pbb_te_label,
#endif /* HAVE_PB_TE */
#endif /* HAVE_GMPLS */
};

/** @brief Triggers the release of a label

    @param zg - daemon specific library global
    @param table - table holding the lp client instances
    @param label_space - label space id
    @param glabel - label to be released
    @param nc - lp client instance
    @param lbl_format - type label technology
    @param serv_module - client or module type
    @param tlv_data_len - any additional technology specific data length
    @param serv_module - pointer to any additional technology specific data

    @return - LP_CLIENT_SUCCESS or LP_CLIENT_FAILURE
*/
int
lp_client_gen_release_label (struct lib_globals *zg,
                        struct route_table *table,
                        u_int16_t lbl_space,
                        struct generalized_label* glabel,
                        struct nsm_client *nc,
                        enum gmpls_label_type  lbl_format,
                        enum label_pool_id serv_module,
                        u_int32_t tlv_data_len,
                        void *tlv_data)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case GMPLS_LABEL_PBB_TE:
#endif /* HAVE_PB_TE */
#endif /* HAVE_GMPLS */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lpc_rel_lbl_pf[lbl_format]) (zg, table, lbl_space, glabel,
                                                  nc, lbl_format, serv_module,
                                                  tlv_data_len, tlv_data);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS */
      default:
        return LP_CLIENT_INVALID_LABEL_TYPE;
        break;
    }
}

int
lp_client_pkt_request_block_send_wrap (struct lib_globals *zg,
                                    struct route_table *table,
                                    u_int16_t label_space,
                                    struct nsm_client *nc,
                                    enum gmpls_label_type  lbl_format,
                                    enum label_pool_id serv_module)
{
  return (lp_client_pkt_request_block_send (zg, table, label_space, nc,
                                            NULL, serv_module));
}

/*
* Function table initilaization to invoke the APIs corresponding to different
* packet technologies as defined in enum gmpls_label_type .
* The funcation table array element must be NULL if a particular label type is
* not supported or the label type doesn't support the respective service, in
* this case send label block request message to NSM.
*/
static int (* const gen_lpc_req_blktx_pf[]) (struct lib_globals *zg,
                                           struct route_table *table,
                                           u_int16_t label_space,
                                           struct nsm_client *nc,
                                           enum gmpls_label_type  lbl_format,
                                           enum label_pool_id serv_module) =
  {
#ifdef HAVE_PACKET
  lp_client_pkt_request_block_send_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PB_TE */
#endif /* HAVE_GMPLS */
  };

/** @brief Sends a label block request message to NSM

    @param zg - daemon specific library global
    @param table - table holding the lp client instances
    @param label_space - label space id
    @param nc - lp client instance
    @param lbl_format - type label technology
    @param serv_module - client or module type

    @return - LP_CLIENT_SUCCESS or LP_CLIENT_FAILURE
*/
int
lp_client_gen_request_block_send (struct lib_globals *zg,
                             struct route_table *table,
                             u_int16_t label_space,
                             struct nsm_client *nc,
                             enum gmpls_label_type  lbl_format,
                             enum label_pool_id serv_module)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lpc_req_blktx_pf[lbl_format]) (zg, table, label_space, nc,
                                                    lbl_format, serv_module);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS */
      default:
        return LP_CLIENT_INVALID_LABEL_TYPE;
        break;
    }
}

int
lp_client_pkt_release_label_cleanup_wrap(struct lib_globals *zg,
                                   struct route_table *table,
                                   u_int16_t label_space,
                                   struct generalized_label* glabel,
                                   struct nsm_client *nc,
                                   int nsm_cleanup,
                                   enum gmpls_label_type  lbl_format,
                                   enum label_pool_id serv_module)
{
  return (lp_client_pkt_release_label_cleanup (zg, table, label_space,
               glabel->u.mpls_label, nc, nsm_cleanup, serv_module));
}

/*
* Function table initilaization to invoke the APIs corresponding to different
* packet technologies as defined in enum gmpls_label_type .
* The funcation table array element must be NULL if a particular label type is
* not supported or the label type doesn't support the respective service, in
* this case trigger a release a label process.
*/
static int (* const gen_lpc_rel_lbl_clnup_pf[]) (struct lib_globals *zg,
                                          struct route_table *table,
                                          u_int16_t label_space,
                                          struct generalized_label* glabel,
                                          struct nsm_client *nc,
                                          int nsm_cleanup,
                                          enum gmpls_label_type  lbl_format,
                                          enum label_pool_id serv_module) =
  {
#ifdef HAVE_PACKET
  lp_client_pkt_release_label_cleanup_wrap,
#endif  /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PB_TE */
#endif /* HAVE_GMPLS */
  };

/** @brief Releases the label and as a result the corresponding internal data

    @param zg - daemon specific library global
    @param table - table holding the lp client instances
    @param label_space - label space id
    @param glabel - generic label to be released
    @param nc - lp client instance
    @param nsm_cleanup - if TRUE, NSM is informed to cleanup respective block
    @param lbl_format - type label technology
    @param serv_module - client or module type

    @return - LP_CLIENT_SUCCESS or a failure code
*/
int
lp_client_gen_release_label_cleanup (struct lib_globals *zg,
                                struct route_table *table,
                                u_int16_t label_space,
                                struct generalized_label* glabel,
                                struct nsm_client *nc,
                                int nsm_cleanup,
                                enum gmpls_label_type  lbl_format,
                                enum label_pool_id serv_module)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lpc_rel_lbl_clnup_pf[lbl_format]) (zg, table, label_space,
                             glabel, nc, nsm_cleanup, lbl_format, serv_module);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS */

      default:
        return LP_CLIENT_INVALID_LABEL_TYPE;
        break;
    }
}

void
lp_client_pkt_free_wrap(struct lib_globals *zg,
                        struct route_table *table,
                        struct nsm_client *nc,
                        enum gmpls_label_type  lbl_format,
                        enum label_pool_id serv_module)
{
  return (lp_client_pkt_free (zg, table, nc, serv_module));
}

/*
* Function table initilaization to invoke the APIs corresponding to different
* packet technologies as defined in enum gmpls_label_type .
* The funcation table array element must be NULL if a particular label type is
* not supported or the label type doesn't support the respective service, in
* this case free the label pool client.
*/
static void (* const gen_lpc_free_pf[]) (struct lib_globals *zg,
                                         struct route_table *table,
                                         struct nsm_client *nc,
                                         enum gmpls_label_type  lbl_format,
                                         enum label_pool_id serv_module) =
  {
#ifdef HAVE_PACKET
  lp_client_pkt_free_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PB_TE */
#endif /* HAVE_GMPLS */
  };

/** @brief Deletes all the label blocks and the client itself

    @param zg - daemon specific library global
    @param table - table holding the lp client instances
    @param nc - lp client instance
    @param lbl_format - type label technology
    @param serv_module - client or module type

    @return - none
*/
void
lp_client_gen_free (struct lib_globals *zg,
                 struct route_table *table,
                 struct nsm_client *nc,
                 enum gmpls_label_type  lbl_format,
                 enum label_pool_id serv_module)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lpc_free_pf[lbl_format]) (zg, table, nc, lbl_format,
                                               serv_module);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS */
      default:
        return;
        break;
    }
}

u_int32_t
lp_client_pkt_get_available_label_count_wrap(struct lib_globals *zg,
                        struct route_table *table,
                        u_int16_t label_space,
                        u_int32_t block_id,
                        enum gmpls_label_type  lbl_format,
                        enum label_pool_id serv_module)
{
  return (lp_client_pkt_get_available_label_count (zg, table, label_space,
                                                   block_id, serv_module));
}

/*
* Function table initilaization to invoke the APIs corresponding to different
* packet technologies as defined in enum gmpls_label_type .
* The funcation table array element must be NULL if a particular label type is
* not supported or the label type doesn't support the respective service, in
* this case get the total number of labels in the list, excluding the block
* corresponding to the block id passed in.
*/
static u_int32_t (* const gen_lpc_get_alc_pf[]) (struct lib_globals *zg,
                                        struct route_table *table,
                                        u_int16_t label_space,
                                        u_int32_t block_id,
                                        enum gmpls_label_type  lbl_format,
                                        enum label_pool_id serv_module) =
        {
#ifdef HAVE_PACKET
  lp_client_pkt_get_available_label_count_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PB_TE */
#endif /* HAVE_GMPLS */
  };

/** @brief Provides the number of labels available in the blocks list excluding
           the block id specified

    @param zg - daemon specific library global
    @param table - table holding the lp client instances
    @param label_space - label space id
    @param block_id - block id to be excluded
    @param lbl_format - type label technology
    @param serv_module - client or module type

    @return - Success: > 0; Failure: 0
*/
u_int32_t
lp_client_gen_get_available_label_count (struct lib_globals *zg,
                                  struct route_table *table,
                                  u_int16_t label_space,
                                  u_int32_t block_id,
                                  enum gmpls_label_type  lbl_format,
                                  enum label_pool_id serv_module)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lpc_get_alc_pf[lbl_format]) (zg, table, label_space,
                                            block_id, lbl_format, serv_module);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS */
      default:
        return LP_CLIENT_INVALID_LABEL_TYPE;
        break;
    }
}

struct lp_client *
lp_client_pkt_lookup_wrap(struct lib_globals *zg,
                        struct route_table *table,
                        u_int16_t label_space,
                        enum gmpls_label_type  lbl_format,
                        enum label_pool_id serv_module)
{
  return (lp_client_pkt_lookup (zg, table, label_space, serv_module));
}

/*
* Function table initilaization to invoke the APIs corresponding to different
* packet technologies as defined in enum gmpls_label_type .
* The funcation table array element must be NULL if a particular label type is
* not supported or the label type doesn't support the respective service, in
* this case get the label pool client specified. If it's not in the table,
* create it
*/
static struct lp_client* (* const gen_lpc_lkup_pf[]) (struct lib_globals *zg,
                                           struct route_table *table,
                                           u_int16_t label_space,
                                           enum gmpls_label_type  lbl_format,
                                           enum label_pool_id serv_module) =
  {
#ifdef HAVE_PACKET
  lp_client_pkt_lookup_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PB_TE */
#endif /* HAVE_GMPLS */
  };

/** @brief Finds the lp client for the specified label space id if it doesn't
           exist in the table, a new one is created

    @param zg - daemon specific library global
    @param table - table holding the lp client instances
    @param label_space - label space id
    @param lbl_format - type label technology
    @param serv_module - client or module type

    @return - Success: lp client instance or NULL otherwise
*/
struct lp_client *
lp_client_gen_lookup (struct lib_globals *zg,
                   struct route_table *table,
                   u_int16_t label_space,
                   enum gmpls_label_type  lbl_format,
                   enum label_pool_id serv_module)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lpc_lkup_pf[lbl_format]) (zg, table, label_space,
                                               lbl_format, serv_module);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS */
      default:
        return NULL;
        break;
    }
}

int
lp_client_pkt_init_label_block_wrap(struct lib_globals *zg,
                             struct route_table *table,
                             u_int16_t label_space,
                             struct nsm_client *nc,
                             enum gmpls_label_type  lbl_format,
                             enum label_pool_id serv_module)
{
  return (lp_client_pkt_init_label_block (zg, table, label_space, nc,
                                          serv_module));
}

/*
* Function table initilaization to invoke the APIs corresponding to different
* packet technologies as defined in enum gmpls_label_type .
* The funcation table array element must be NULL if a particular label type is
* not supported or the label type doesn't support the respective service, in
* this case request the first block during initialization of an interface on
* the protocol end.
*/
static int (* const gen_lpc_init_blk_pf[]) (struct lib_globals *zg,
                                          struct route_table *table,
                                          u_int16_t label_space,
                                          struct nsm_client *nc,
                                          enum gmpls_label_type  lbl_format,
                                          enum label_pool_id serv_module) =
{
#ifdef HAVE_PACKET
  lp_client_pkt_init_label_block_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PB_TE */
#endif /* HAVE_GMPLS */
};

/** @brief Initializes the first block in response to protocol initialization
           of an interafce

    @param zg - daemon specific library global
    @param table - table holding the lp client instances
    @param label_space - label space id
    @param nc - lp client instance
    @param lbl_format - type label technology
    @param serv_module - client or module type

    @return - LP_CLIENT_SUCCESS or LP_CLIENT_FAILURE
*/
int
lp_client_gen_init_label_block (struct lib_globals *zg,
                          struct route_table *table,
                          u_int16_t label_space,
                          struct nsm_client *nc,
                          enum gmpls_label_type  lbl_format,
                          enum label_pool_id serv_module)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lpc_init_blk_pf[lbl_format]) (zg, table, label_space,
                                                   nc, lbl_format, serv_module);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS */
      default:
        return LP_CLIENT_INVALID_LABEL_TYPE;
        break;
    }
}

void
lp_client_pkt_deinit_wrap(struct lib_globals *zg,
                     struct route_table *table,
                     u_int16_t label_space,
                     struct nsm_client *nc,
                     enum gmpls_label_type  lbl_format,
                     enum label_pool_id serv_module)
{
  return (lp_client_pkt_deinit (zg, table, label_space, nc, serv_module));
}

/*
* Function table initilaization to invoke the APIs corresponding to different
* packet technologies as defined in enum gmpls_label_type .
* The funcation table array element must be NULL if a particular label type is
* not supported or the label type doesn't support the respective service, in
* this case de-initialize the label pool client.
*/
static void (* const gen_lpc_deinit_pf[]) (struct lib_globals *zg,
                                          struct route_table *table,
                                          u_int16_t label_space,
                                          struct nsm_client *nc,
                                          enum gmpls_label_type  lbl_format,
                                          enum label_pool_id serv_module) =
  {
#ifdef HAVE_PACKET
  lp_client_pkt_deinit_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PB_TE */
#endif /* HAVE_GMPLS */
  };

/** @brief Releases label pool client instance

    @param zg - daemon specific library global
    @param table - table holding the lp client instances
    @param label_space - label space id
    @param nc - lp client instance
    @param lbl_format - type label technology
    @param serv_module - client or module type

    @return - none
*/
void
lp_client_gen_deinit (struct lib_globals *zg,
                  struct route_table *table,
                  u_int16_t label_space,
                  struct nsm_client *nc,
                  enum gmpls_label_type  lbl_format,
                  enum label_pool_id serv_module)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lpc_deinit_pf[lbl_format]) (zg, table, label_space,
                                                 nc, lbl_format, serv_module);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS */

      default:
        return;
        break;
    }
}

int
lp_client_pkt_is_usable_wrap(struct lib_globals *zg,
                        struct route_table *table,
                        u_int16_t label_space,
                        u_int32_t block_id,
                        struct nsm_client *nc,
                        enum gmpls_label_type  lbl_format,
                        enum label_pool_id serv_module,
                        u_int32_t tlv_data_len,
                        void *tlv_data)
{
  return (lp_client_pkt_is_usable (zg, table, label_space, block_id,
                                   serv_module));
}

/*
* Function table initilaization to invoke the APIs corresponding to different
* packet technologies as defined in enum gmpls_label_type .
* The funcation table array element must be NULL if a particular label type is
* not supported or the label type doesn't support the respective service, in
* this case check if there is atleast one label available.
*/
static int (* const gen_lpc_usable_pf[]) (struct lib_globals *zg,
                                         struct route_table *table,
                                         u_int16_t label_space,
                                         u_int32_t block_id,
                                         struct nsm_client *nc,
                                         enum gmpls_label_type  lbl_format,
                                         enum label_pool_id serv_module,
                                         u_int32_t tlv_data_len,
                                         void *tlv_data) =
  {
#ifdef HAVE_PACKET
  lp_client_pkt_is_usable_wrap,
#endif  /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  lp_client_pbb_te_usable,
#endif /* HAVE_PB_TE */
#endif /* HAVE_GMPLS */
  };

/** @brief Verifies if there is at least one label available for use excluding
           the block id specified

    @param zg - daemon specific library global
    @param table - table holding the lp client instances
    @param label_space - label space id
    @param block_id - block id to be excluded
    @param nc - lp client instance
    @param nsm_cleanup - if TRUE, NSM message is dispatched
    @param lbl_format - type label technology
    @param serv_module - client or module type
    @param tlv_data_len - any additional technology specific data length
    @param serv_module - pointer to any additional technology specific data

    @return - PAL_TRUE or PAL_FALSE
*/
int
lp_client_gen_is_usable (struct lib_globals *zg,
                    struct route_table *table,
                    u_int16_t label_space,
                    u_int32_t block_id,
                    struct nsm_client *nc,
                    enum gmpls_label_type  lbl_format,
                    enum label_pool_id serv_module,
                    u_int32_t tlv_data_len,
                    void *tlv_data)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case GMPLS_LABEL_PBB_TE:
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lpc_usable_pf[lbl_format]) (zg, table, label_space,
                     block_id, nc, lbl_format, serv_module, tlv_data_len,
                     tlv_data);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS */

      default:
        return LP_CLIENT_INVALID_LABEL_TYPE;
        break;
    }
}

void
lp_client_pkt_update_request_failure_wrap (struct lib_globals *zg,
                                    struct route_table *table,
                                    struct lp_client *client,
                                    u_char type,
                                    enum gmpls_label_type  lbl_format,
                                    enum label_pool_id serv_module)
{
  return (lp_client_pkt_update_request_failure (zg, table, client, type,
                                                serv_module));
}

/*
* Function table initilaization to invoke the APIs corresponding to different
* packet technologies as defined in enum gmpls_label_type .
* The funcation table array element must be NULL if a particular label type is
* not supported or the label type doesn't support the respective service, in
* this case update the stats for relese or request failure.
*/
static void (* const gen_lpc_urf_pf[]) (struct lib_globals *zg,
                                         struct route_table *table,
                                         struct lp_client *client,
                                         u_char type,
                                         enum gmpls_label_type  lbl_format,
                                         enum label_pool_id serv_module) =
  {
#ifdef HAVE_PACKET
  lp_client_pkt_update_request_failure_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PB_TE */
#endif /* HAVE_GMPLS */
  };

/** @brief Updates failure count based on failure type

    @param zg - daemon specific library global
    @param table - table holding the lp client instances
    @param client - lp client instance
    @param type - request failure type - REQUEST or RELEASE
    @param lbl_format - type label technology
    @param serv_module - client or module type

    @return - none
*/
void
lp_client_gen_update_request_failure (struct lib_globals *zg,
                               struct route_table *table,
                               struct lp_client *client,
                               u_char type,
                               enum gmpls_label_type  lbl_format,
                               enum label_pool_id serv_module)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lpc_urf_pf[lbl_format]) (zg, table, client, type,
                                              lbl_format, serv_module);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS */
      default:
        return;
        break;
    }
}

int
lp_client_pkt_bind_block_wrap(struct lib_globals *zg,
                        struct route_table *table,
                        u_int16_t label_space,
                        u_int32_t block,
                        struct generalized_label* gmin_label,
                        struct generalized_label* gmax_label,
                        enum gmpls_label_type  lbl_format,
                        enum label_pool_id serv_module)
{
  return (lp_client_pkt_bind_block (zg, table, label_space, block,
                         gmin_label->u.mpls_label,
                         gmax_label->u.mpls_label, serv_module));
}

/*
* Function table initilaization to invoke the APIs corresponding to different
* packet technologies as defined in enum gmpls_label_type .
* The funcation table array element must be NULL if a particular label type is
* not supported or the label type doesn't support the respective service, in
* this case process a bind request's reply.
*/
static int (* const gen_lpc_bind_blk_pf[]) (struct lib_globals *zg,
                                    struct route_table *table,
                                    u_int16_t label_space,
                                    u_int32_t block,
                                    struct generalized_label* gmin_label,
                                    struct generalized_label* gmax_label,
                                    enum gmpls_label_type  lbl_format,
                                    enum label_pool_id serv_module) =
{
#ifdef HAVE_PACKET
  lp_client_pkt_bind_block_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PB_TE */
#endif /* HAVE_GMPLS */
};

/** @brief Upon successful block allocation, the lp client creates a bit map
           to track the labels comprising the block and adds it to the block
           list

    @param zg - daemon specific library global
    @param table - table holding the lp client instances
    @param label_space - label space id
    @param block - block id to be added to the block list
    @param gmin_label - min of label space
    @param gmin_label - max of label space
    @param lbl_format - type label technology
    @param serv_module - client or module type

    @return - LP_CLIENT_SUCCESS or LP_CLIENT_FAILURE
*/
int
lp_client_gen_bind_block (struct lib_globals *zg,
                      struct route_table *table,
                      u_int16_t label_space,
                      u_int32_t block,
                      struct generalized_label* gmin_label,
                      struct generalized_label* gmax_label,
                      enum gmpls_label_type  lbl_format,
                      enum label_pool_id serv_module)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lpc_bind_blk_pf[lbl_format]) (zg, table, label_space,
                                                  block, gmin_label, gmax_label,
                                                  lbl_format, serv_module);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS */
      default:
        return LP_CLIENT_INVALID_LABEL_TYPE;
        break;
    }
}

#ifdef HAVE_GMPLS

int
lp_client_pkt_get_label_from_label_set_wrap (struct lib_globals *zg,
                                     struct route_table *table,
                                     u_int16_t lbl_space,
                                     struct nsm_client *nc,
                                     struct lp_client_label_set *label_set,
                                     struct generalized_label *glabel_ret,
                                     enum gmpls_label_type  lbl_format,
                                     enum label_pool_id serv_module)
{
  int rc;

  rc = lp_client_pkt_get_label_from_label_set (zg, table,
                                                  lbl_space, nc, label_set,
                                                  glabel_ret, serv_module);
  if (rc < 0 )
    {
      glabel_ret->gmpls_label_type = GMPLS_LABEL_ERROR;
      glabel_ret->u.mpls_label = LABEL_VALUE_INVALID;
    }

  return rc;
}

/*
* Function table initilaization to invoke the APIs corresponding to different
* packet technologies as defined in enum gmpls_label_type .
* The funcation table array element must be NULL if a particular label type is
* not supported or the label type doesn't support the respective service, in
* this case provides a single label from the input label set that can be used
* by the client.
*/
static int (* const gen_lpc_glfls_pf[]) (struct lib_globals *zg,
                                         struct route_table *table,
                                         u_int16_t lbl_space,
                                         struct nsm_client *nc,
                                         struct lp_client_label_set *label_set,
                                         struct generalized_label *glabel_ret,
                                         enum gmpls_label_type lbl_format,
                                         enum label_pool_id serv_module) =
{
#ifdef HAVE_PACKET
  lp_client_pkt_get_label_from_label_set_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
};

/** @brief Provides the abiity to request a label over a set of one or more
           label set objects as per RFC 3471

    @param zg - daemon specific library global
    @param table - table holding the lp client instances
    @param label_space - label space id
    @param nc - lp client instance
    @param lbl_set - label set over which the label is requested
    @param gen_label - the label value being returned
    @param nsm_cleanup - if TRUE, NSM message is dispatched
    @param lbl_format - type label technology
    @param serv_module - client or module type

    @return - Success: LP_CLIENT_PKT_LBL_MATCH_FOUND with value in glabel_ret
              Failure: LP_CLIENT_PKT_LBL_MATCH_NOT_FOUND with label type in
                       glabel_ret set to GMPLS_LABEL_ERROR
*/
int
lp_client_gen_get_label_from_label_set (struct lib_globals *zg,
                                 struct route_table *table,
                                 u_int16_t lbl_space,
                                 struct nsm_client *nc,
                                 struct lp_client_label_set *label_set,
                                 struct generalized_label *glabel_ret,
                                 enum gmpls_label_type lbl_format,
                                 enum label_pool_id serv_module)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lpc_glfls_pf[lbl_format]) (zg, table, lbl_space, nc,
                              label_set, glabel_ret, lbl_format, serv_module);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS*/
      default:
        return LP_CLIENT_INVALID_LABEL_TYPE;
    }
}

int
lp_client_pkt_get_label_by_value_wrap (struct lib_globals *zg,
                                 struct route_table *table,
                                 u_int16_t lbl_space,
                                 struct nsm_client *nc,
                                 u_int8_t type_of_service,
                                 struct generalized_label* gen_label,
                                 struct lp_client_label_set *lbl_set,
                                 enum gmpls_label_type  lbl_format,
                                 enum label_pool_id serv_module)
{
  return (lp_client_pkt_get_label_by_value (zg, table, lbl_space, nc,
                                            type_of_service,
                                            gen_label, lbl_set, serv_module));
}

/*
* Function table initilaization to invoke the APIs corresponding to different
* packet technologies as defined in enum gmpls_label_type .
* The funcation table array element must be NULL if a particular label type is
* not supported or the label type doesn't support the respective service, in
* this case perform the type of service requested by the client on an
* individual label.
*/
static int (* const gen_lpc_glbv_pf[]) (struct lib_globals *zg,
                                        struct route_table *table,
                                        u_int16_t lbl_space,
                                        struct nsm_client *nc,
                                        u_int8_t type_of_service,
                                        struct generalized_label* gen_label,
                                        struct lp_client_label_set *lbl_set,
                                        enum gmpls_label_type  lbl_format,
                                        enum label_pool_id serv_module) =
{
#ifdef HAVE_PACKET
  lp_client_pkt_get_label_by_value_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
};

/** @brief This API is used to get the specified label if it is available, for
           example, the RSVP explicit, suggesetd label request

    @param zg - daemon specific library global
    @param table - table holding the lp client instances
    @param label_space - label space id
    @param nc - lp client instance
    @param type_of_service - indicate the service request type as above
    @param gen_label - the label value being requested
    @param lbl_set - label set over which the label is requested
    @param lbl_format - type label technology
    @param serv_module - client or module type

    @return - Success: LP_CLIENT_PKT_LBL_MATCH_FOUND
              Failure: LP_CLIENT_PKT_LBL_MATCH_NOT_FOUND or LABEL_VALUE_INVALID
*/
int
lp_client_gen_get_label_by_value (struct lib_globals *zg,
                             struct route_table *table,
                             u_int16_t lbl_space,
                             struct nsm_client *nc,
                             u_int8_t type_of_service,
                             struct generalized_label* gen_label,
                             struct lp_client_label_set *lbl_set,
                             enum gmpls_label_type  lbl_format,
                             enum label_pool_id serv_module)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lpc_glbv_pf[lbl_format]) (zg, table, lbl_space, nc,
                                              type_of_service, gen_label,
                                              lbl_set, lbl_format, serv_module);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS*/
      default:
        return LP_CLIENT_INVALID_LABEL_TYPE;
        break;
    }
}

#endif /* HAVE_GMPLS */

#if defined HAVE_RSVP_GRST || defined HAVE_RESTART

int
lp_client_pkt_reserve_label_wrap(struct lib_globals *zg,
                            struct route_table *table,
                            u_int16_t lbl_space,
                            struct generalized_label* lbl,
                            enum gmpls_label_type  lbl_format,
                            enum label_pool_id serv_module)
{
  if (lbl)
  {
    return (lp_client_pkt_reserve_label (zg, table, lbl_space,
                                   lbl->u.mpls_label, serv_module));
  }
  return 0;
}

/*
* Function table initilaization to invoke the APIs corresponding to different
* packet technologies as defined in enum gmpls_label_type .
* The funcation table array element must be NULL if a particular label type is
* not supported or the label type doesn't support the respective service, in
* this case reserve a label for graceful restart.
*/
static int (* const gen_lpc_resv_lbl_pf[]) (struct lib_globals *zg,
                                            struct route_table *table,
                                            u_int16_t label_space,
                                            struct generalized_label* glabel,
                                            enum gmpls_label_type  lbl_format,
                                            enum label_pool_id serv_module) =
{
#ifdef HAVE_PACKET
  lp_client_pkt_reserve_label_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PB_TE */
#endif /* HAVE_GMPLS */
};

/** @brief Reserves a label as part of graceful restart recovery procedure

    @param zg - daemon specific library global
    @param table - table holding the lp client instances
    @param label_space - label space id
    @param glabel - particular label to be reserved for graceful restart
    @param lbl_format - type label technology
    @param serv_module - client or module type

    @return - LP_CLIENT_SUCCESS or LP_CLIENT_FAILURE
*/
int
lp_client_gen_reserve_label (struct lib_globals *zg,
                        struct route_table *table,
                        u_int16_t label_space,
                        struct generalized_label* glabel,
                        enum gmpls_label_type  lbl_format,
                        enum label_pool_id serv_module)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lpc_resv_lbl_pf[lbl_format]) (zg, table, label_space,
                                             glabel, lbl_format, serv_module);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS */
      default:
        return LP_CLIENT_INVALID_LABEL_TYPE;
        break;
    }
}

int
lp_client_pkt_mark_block_stale_wrap(struct lib_globals *zg,
                               struct route_table *table,
                               u_int16_t lbl_space,
                               bool_t mark_stale,
                               enum gmpls_label_type  lbl_format,
                               enum label_pool_id serv_module)
{
  return (lp_client_pkt_mark_block_stale (zg, table, lbl_space,
                                          mark_stale, serv_module));
}

/*
* Function table initilaization to invoke the APIs corresponding to different
* packet technologies as defined in enum gmpls_label_type .
* The funcation table array element must be NULL if a particular label type is
* not supported or the label type doesn't support the respective service, in
* this case mark a block as stale or unstale for graceful restart.
*/
static int (* const gen_lpc_mark_bs_pf[]) (struct lib_globals *zg,
                                          struct route_table *table,
                                          u_int16_t label_space,
                                          bool_t mark_as_stale,
                                          enum gmpls_label_type  lbl_format,
                                          enum label_pool_id serv_module
                                          ) =
{
#ifdef HAVE_PACKET
  lp_client_pkt_mark_block_stale_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PB_TE */
#endif /* HAVE_GMPLS */
};

/** @brief Marks the block as stale; used specifically for graceful resetrt

    @param zg - daemon specific library global
    @param table - table holding the lp client instances
    @param label_space - label space id
    @param mark_stale - if set, the block is marked as stale
    @param lbl_format - type label technology
    @param serv_module - client or module type

    @return - LP_CLIENT_SUCCESS or LP_CLIENT_FAILURE
*/
int
lp_client_gen_mark_block_stale (struct lib_globals *zg,
                      struct route_table *table,
                      u_int16_t label_space,
                      bool_t mark_stale,
                      enum gmpls_label_type  lbl_format,
                      enum label_pool_id serv_module)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lpc_mark_bs_pf[lbl_format]) (zg, table, label_space,
                                                  mark_stale, lbl_format,
                                                  serv_module);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS */
      default:
        return LP_CLIENT_INVALID_LABEL_TYPE;
        break;
    }
}

#endif /* HAVE_RSVP_GRST  || HAVE_RESTART */

