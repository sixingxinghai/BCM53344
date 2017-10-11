/* Copyright (C) 2001-2009  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "vty.h"
#include "prefix.h"
#include "table.h"
#include "log.h"
#include "label_pool.h"
#include "if.h"
#include "mpls.h"
#include "mpls_common.h"
#include "nsmd.h"
#ifdef HAVE_DSTE
#include "dste.h"
#include "nsm_dste.h"
#endif /* HAVE_DSTE */
#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */
#include "nsm_mpls.h"
#include "nsm_lp_serv.h"
#include "nsm_interface.h"
#include "nsm_message.h"
#include "nsm_lp_serv_packet.h"


/* Maximum blocks for a server node. */
/* Kept for legacy sake */
/*
static int
nsm_lp_server_max_blocks (struct lp_server *server)
{
  return (nsm_lp_server_gen_max_blocks (server, GMPLS_LABEL_PACKET));
}
*/

/* Create a new label pool server for a given label space */
struct lp_server*
nsm_lp_server_new (struct nsm_master *nm,
                 u_int16_t label_space)
{
  return (nsm_lp_server_gen_new (nm, label_space,
                                 LABEL_POOL_SERVICE_UNSUPPORTED,
                                 GMPLS_LABEL_PACKET));
}

/*
 * Get the label pool server specified. If it's not in the
 * table, create it.
 */
struct lp_server *
nsm_lp_server_get (struct nsm_master *nm,
                u_int16_t label_space,
                int create)
{
  return (nsm_lp_server_gen_get (nm, label_space, create,
                        LABEL_POOL_SERVICE_UNSUPPORTED, GMPLS_LABEL_PACKET));
}

/* Helper function to find the next free block. */
int
nsm_lp_server_find_next_free (struct lp_server *server)
{
  return (nsm_lp_server_gen_find_next_free (server, GMPLS_LABEL_PACKET));
}

/*
 * The following routine returns the first free block available.
 */
u_int32_t
nsm_lp_server_get_block (struct nsm_master *nm,
                     u_int16_t label_space,
                     u_char protocol)
{
  return (nsm_lp_server_gen_get_block (nm, label_space, protocol, NULL,
                                       NULL, NULL,
                                       LABEL_POOL_SERVICE_UNSUPPORTED,
                                       GMPLS_LABEL_PACKET));
}

u_int32_t
nsm_lp_static_block_num_get (u_int32_t min,
                         u_int32_t max)
{
  return (nsm_lp_server_gen_static_block_num_get (min, max, GMPLS_LABEL_PACKET));
}

void
nsm_lp_static_block_get (struct nsm_master *nm,
                     struct nsm_label_space *nls,
                     u_char protocol)
{
  return (nsm_lp_server_gen_static_block_get (nm, nls, protocol,
                                              LABEL_POOL_SERVICE_UNSUPPORTED,
                                              GMPLS_LABEL_PACKET));
}

/* Free the specific label pool server */
void
nsm_lp_server_free (struct nsm_master *nm,
                 struct route_node *rn)
{
  return (nsm_lp_server_gen_free (nm, rn, GMPLS_LABEL_PACKET));
}

/*
 * Free the block number specified.
 */
int
nsm_lp_server_free_block (struct nsm_master *nm,
                      u_int16_t label_space,
                      u_int32_t block,
                      u_char protocol)
{
  return (nsm_lp_server_gen_free_block (nm, label_space, block, protocol,
                                        LABEL_POOL_SERVICE_UNSUPPORTED,
                                        GMPLS_LABEL_PACKET));
}

void
nsm_lp_static_block_release (struct nsm_master *nm,
                        struct nsm_label_space *nls,
                        u_char protocol)
{
  return (nsm_lp_server_static_block_release (nm, nls, protocol));
}

/* Figure out the minimum and maximum label values for this server. */
u_int32_t
nsm_lp_server_get_label_value (struct nsm_master *nm,
                          u_int16_t label_space,
                          int get_maximum)
{
  return (nsm_lp_server_gen_get_label_value (nm, label_space, get_maximum,
                                             LABEL_POOL_SERVICE_UNSUPPORTED,
                                             GMPLS_LABEL_PACKET));
}

/* Figure out if the lp server corresponding to the interface index
   passed in is in use. If argument is 0xffffffff, check if ANY labelspaces
   are in use. */
int
nsm_lp_server_in_use (struct nsm_master *nm,
                   u_int32_t label_space)
{
  return (nsm_lp_server_gen_in_use (nm, label_space,
                          LABEL_POOL_SERVICE_UNSUPPORTED, GMPLS_LABEL_PACKET));
}


/* Clean up everything pertaining to the passed in protocol that have specified
 * status. */
void
nsm_lp_server_clean_for_w_status (struct nsm_master *nm,
                             u_char protocol,
                             u_char status)
{
  return (nsm_lp_server_gen_clean_for_w_status (nm, protocol, status,
                                                LABEL_POOL_SERVICE_UNSUPPORTED,
                                                GMPLS_LABEL_PACKET));
}
/* Clean up everything pertaining to the passed in protocol that are in use. */
void
nsm_lp_server_clean_in_use_for (struct nsm_master *nm, u_char protocol)
{
  return (nsm_lp_server_gen_clean_in_use_for (nm, protocol,
                            LABEL_POOL_SERVICE_UNSUPPORTED, GMPLS_LABEL_PACKET));
}

/* Clean up everything pertaining to the passed in protocol that are stale. */
void
nsm_lp_server_clean_stale_for (struct nsm_master *nm, u_char protocol)
{
  return (nsm_lp_server_gen_clean_stale_for (nm, protocol,
                            LABEL_POOL_SERVICE_UNSUPPORTED, GMPLS_LABEL_PACKET));
}

/* Mark entries pertaining to the passed in protocol with stale flag. */
void
nsm_lp_server_set_stale_for (struct nsm_master *nm, u_char protocol)
{
  return (nsm_lp_server_gen_set_stale_for (nm, protocol,
                            LABEL_POOL_SERVICE_UNSUPPORTED, GMPLS_LABEL_PACKET));
}

/* Get stale entries pertaining to the passed in protocol, construct a list
   of "struct nsm_msg_label_pool" type to be returned to the client, and
   unstale the entries. */
void
nsm_lp_server_get_stale_for (struct nsm_master *nm,
                             struct nsm_msg_service *message,
                             u_char protocol,
                             struct nsm_msg_label_pool **label_pools,
                             u_int16_t *label_pool_num)
{
  return (nsm_lp_server_gen_get_stale_for (nm, message, protocol, label_pools,
                                           label_pool_num,
                                           LABEL_POOL_SERVICE_UNSUPPORTED,
                                           GMPLS_LABEL_PACKET));
}

u_int32_t
nsm_lp_server_static_block_num_get (u_int32_t min, u_int32_t max)
{
  u_int32_t label_num, bn;

  label_num = (MPLS_STATIC_LABEL_RATIO * (max - min)) / 100 + min;

  bn = label_num / PACKET_LABEL_BUCKET_SIZE;

  if ((label_num % PACKET_LABEL_BUCKET_SIZE) > 0)
    bn++;

  return bn;
}

void
nsm_lp_server_static_block_get (struct nsm_master *nm,
                                struct nsm_label_space *nls,
                                u_char protocol)
{
  int i;
  u_int32_t block_num;

  block_num = nsm_lp_static_block_num_get (nls->min_label_val,
                                           nls->max_label_val);

  for (i = 0; i < block_num; i++)
    {
      nsm_lp_server_gen_get_block (nm, nls->label_space, protocol,
                                   NULL, NULL, NULL,
                                   LABEL_POOL_SERVICE_UNSUPPORTED,
                                   GMPLS_LABEL_PACKET);
    }
}

void
nsm_lp_server_static_block_release (struct nsm_master *nm, struct nsm_label_space *nls,
                             u_char protocol)
{
  int i;
  u_int32_t block_num;

  block_num = nsm_lp_static_block_num_get (nls->min_label_val,
                                           nls->max_label_val);

  for (i = 0; i < block_num; i++)
    nsm_lp_server_gen_free_block (nm, nls->label_space, i, protocol,
                                  LABEL_POOL_SERVICE_UNSUPPORTED,
                                  GMPLS_LABEL_PACKET);
}

/* This function returns the index into the service range array given the
 * service type bit value
 */
int
nsm_lp_server_get_service_range_index (u_int8_t range_owner_id)
{
  switch (range_owner_id)
    {
      case LABEL_POOL_SERVICE_RSVP:
        return 0;

      case LABEL_POOL_SERVICE_LDP:
        return 1;

      case LABEL_POOL_SERVICE_LDP_LDP_PW:
        return 2;

      case LABEL_POOL_SERVICE_LDP_VPLS:
        return 3;

      case LABEL_POOL_SERVICE_BGP:
        return 4;

      default:
        return LABEL_POOL_SERVICE_ERROR;
    }
}

/* This function returns the service type bit value given the index into the
 * service range array.
 */
u_int8_t
nsm_lp_server_get_service_range_type (int serv_index)
{
  switch (serv_index)
    {
      case 0:
        return LABEL_POOL_SERVICE_RSVP;

      case 1:
        return LABEL_POOL_SERVICE_LDP;

      case 2:
        return LABEL_POOL_SERVICE_LDP_LDP_PW;

      case 3:
        return LABEL_POOL_SERVICE_LDP_VPLS;

      case 4:
        return LABEL_POOL_SERVICE_BGP;

      default:
        return LABEL_POOL_SERVICE_UNSUPPORTED;
    }
}

/**********************************************************************
 *
 *                    GMPLS LABEL POOL SERVER SECTION
 *
 **********************************************************************/

int
nsm_lp_server_max_blocks_wrap (struct lp_server *server,
                            enum gmpls_label_type  lbl_format)
{
  return (nsm_lp_server_pkt_max_blocks (server));
}

/*
* Function table initilaization to invoke the APIs corresponding to different
* packet technologies as defined in enum gmpls_label_type .
* The funcation table array element must be NULL if a particular label type is
* not supported or the label type doesn't support respective service, in
* this case finding the maximum blocks for the server node.
*/
static int (* const gen_lps_max_blocks_pf[]) (struct lp_server *server,
                                      enum gmpls_label_type  lbl_format) =
  {
#ifdef HAVE_PACKET
  nsm_lp_server_max_blocks_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  };

int
nsm_lp_server_gen_max_blocks (struct lp_server *server,
                           enum gmpls_label_type  lbl_format)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lps_max_blocks_pf[lbl_format]) (server, lbl_format);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS */
      default:
        return LP_SERVER_UNDEFINED_LBL_TYPE;
        break;
    }
}

struct lp_server*
nsm_lp_server_new_wrap (struct nsm_master *nm,
                      u_int16_t label_space,
                      u_int8_t range_owner,
                      enum gmpls_label_type  lbl_format)
{
  return (nsm_lp_server_pkt_new (nm, label_space, range_owner,
                                 GMPLS_LABEL_PACKET));
}

/*
* Function table initilaization to invoke the APIs corresponding to different
* packet technologies as defined in enum gmpls_label_type .
* The funcation table array element must be NULL if a particular label type is
* not supported or the label type doesn't support respective service, in
* this case creating a new label pool server for a given label space
*/
static struct lp_server* (* const gen_lps_new_pf[]) (struct nsm_master *nm,
                                     u_int16_t label_space,
                                     u_int8_t range_owner,
                                     enum gmpls_label_type  lbl_format) =
  {
#ifdef HAVE_PACKET
    nsm_lp_server_new_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  };

struct lp_server*
nsm_lp_server_gen_new (struct nsm_master *nm,
                     u_int16_t label_space,
                     u_int8_t range_owner,
                     enum gmpls_label_type  lbl_format)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lps_new_pf[lbl_format]) (nm, label_space, range_owner,
                                              lbl_format);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS */
      default:
        return NULL;
        break;
    }
}

struct lp_server*
nsm_lp_server_get_wrap (struct nsm_master *nm,
                      u_int16_t label_space,
                      int create,
                      u_int8_t range_owner,
                      enum gmpls_label_type  lbl_format)
{
  return (nsm_lp_server_pkt_get (nm, label_space, create, range_owner,
                                 lbl_format));
}

/*
* Function table initilaization to invoke the APIs corresponding to different
* packet technologies as defined in enum gmpls_label_type .
* The funcation table array element must be NULL if a particular label type is
* not supported or the label type doesn't support respective service, in
* this case seeking the label pool server.
*/
static struct lp_server* (* const gen_lps_get_pf[]) (struct nsm_master *nm,
                                    u_int16_t label_space,
                                    int create,
                                    u_int8_t range_owner,
                                    enum gmpls_label_type  lbl_format) =
  {
#ifdef HAVE_PACKET
  nsm_lp_server_get_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  };

struct lp_server *
nsm_lp_server_gen_get (struct nsm_master *nm,
                    u_int16_t label_space,
                    int create,
                    u_int8_t range_owner,
                    enum gmpls_label_type  lbl_format)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lps_get_pf[lbl_format]) (nm, label_space, create,
                                              range_owner, lbl_format);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS */
      default:
        return NULL;
        break;
    }
}

int
nsm_lp_server_find_next_free_wrap (struct lp_server *server,
                     enum gmpls_label_type  lbl_format)
{
  return (nsm_lp_server_find_next_free (server));
}

/*
 * Function table initilaization to invoke the APIs corresponding to different
 * packet technologies as defined in enum gmpls_label_type .
 * The funcation table array element must be NULL if a particular label type is
 * not supported or the label type doesn't support the respective service, in
 * this to find the next free block.
 */
static int (* const gen_lps_next_free_pf[]) (struct lp_server *server,
                                  enum gmpls_label_type  lbl_format) =
  {
#ifdef HAVE_PACKET
  nsm_lp_server_find_next_free_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  };

int
nsm_lp_server_gen_find_next_free (struct lp_server *server,
                             enum gmpls_label_type  lbl_format)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lps_next_free_pf[lbl_format]) (server, lbl_format);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS*/
      default:
        return LP_SERVER_UNDEFINED_LBL_TYPE;
        break;
    }
}

u_int32_t
nsm_lp_server_pkt_get_block_wrap(struct nsm_master *nm,
                             u_int16_t label_space,
                             u_char protocol,
                             struct nsm_msg_pkt_block_list *blist,
                             struct lp_serv_lset_block *ls_blk,
                             u_int8_t *range_ret_data,
                             u_int8_t range_owner,
                             enum gmpls_label_type  lbl_format)
{
  return (nsm_lp_server_pkt_get_block (nm, label_space, protocol, blist,
                                       ls_blk, range_ret_data,
                                       range_owner, lbl_format));
}

/*
 * Function table initilaization to invoke the APIs corresponding to different
 * packet technologies as defined in enum gmpls_label_type .
 * The funcation table array element must be NULL if a particular label type is
 * not supported or the label type doesn't support the respective service, in
 * this case gets the first free block available.
 */
static u_int32_t (* const gen_lps_get_block_pf[]) (struct nsm_master *nm,
                                   u_int16_t label_space,
                                   u_char protocol,
                                   struct nsm_msg_pkt_block_list *blist,
                                   struct lp_serv_lset_block *ls_blk,
                                   u_int8_t *range_ret_data,
                                   u_int8_t range_owner,
                                   enum gmpls_label_type  lbl_format) =
  {
#ifdef HAVE_PACKET
  nsm_lp_server_pkt_get_block_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  };

u_int32_t
nsm_lp_server_gen_get_block (struct nsm_master *nm,
                          u_int16_t label_space,
                          u_char protocol,
                          struct nsm_msg_pkt_block_list *blist,
                          struct lp_serv_lset_block *ls_blk,
                          u_int8_t *range_ret_data,
                          u_int8_t range_owner,
                          enum gmpls_label_type  lbl_format)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lps_get_block_pf[lbl_format]) (nm, label_space, protocol,
                             blist, ls_blk, range_ret_data,
                             range_owner, lbl_format);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS*/
      default:
        return LP_SERVER_UNDEFINED_LBL_TYPE;
        break;
    }
}

u_int32_t
nsm_lp_server_static_block_num_get_wrap (u_int32_t min,
                                    u_int32_t max,
                                    enum gmpls_label_type  lbl_format)
{
  return (nsm_lp_server_static_block_num_get (min, max));
}

/*
 * Function table initilaization to invoke the APIs corresponding to different
 * packet technologies as defined in enum gmpls_label_type .
 * The funcation table array element must be NULL if a particular label type is
 * not supported or the label type doesn't support the respective service, in
 * this case get a static block within the given range.
 */
static u_int32_t (* const gen_lps_get_statnum_blk_pf[]) (u_int32_t min,
                                   u_int32_t max,
                                   enum gmpls_label_type  lbl_format) =
  {
#ifdef HAVE_PACKET
  nsm_lp_server_static_block_num_get_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  };

u_int32_t
nsm_lp_server_gen_static_block_num_get (u_int32_t min,
                                   u_int32_t max,
                                   enum gmpls_label_type  lbl_format)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lps_get_statnum_blk_pf[lbl_format]) (min, max, lbl_format);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS */
      default:
        return LP_SERVER_UNDEFINED_LBL_TYPE;
        break;
    }
}

void
nsm_lp_server_static_block_get_wrap (struct nsm_master *nm,
                                    struct nsm_label_space *label_space,
                                    u_char protocol,
                                    u_int8_t range_owner,
                                    enum gmpls_label_type  lbl_format)
{
  return (nsm_lp_server_static_block_get (nm, label_space, protocol));
}

/*
 * Function table initilaization to invoke the APIs corresponding to different
 * packet technologies as defined in enum gmpls_label_type .
 * The funcation table array element must be NULL if a particular label type is
 * not supported or the label type doesn't support the respective service, in
 * this case a obtaining a block of labels from the label pool server.
 */
static void (* const gen_get_stat_block_pf[]) (struct nsm_master *nm,
                                     struct nsm_label_space *label_space,
                                     u_char protocol,
                                     u_int8_t range_owner,
                                     enum gmpls_label_type  lbl_format) =
  {
#ifdef HAVE_PACKET
  nsm_lp_server_static_block_get_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  };

void
nsm_lp_server_gen_static_block_get (struct nsm_master *nm,
                              struct nsm_label_space *label_space,
                              u_char protocol,
                              u_int8_t range_owner,
                              enum gmpls_label_type  lbl_format)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_get_stat_block_pf[lbl_format]) (nm, label_space, protocol,
                                                     range_owner, lbl_format);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS */
      default:
        return;
        break;
    }
}

void
nsm_lp_server_free_wrap (struct nsm_master *nm,
                      struct route_node *rn,
                      enum gmpls_label_type  lbl_format)
{
  return (nsm_lp_server_pkt_free (nm, rn, lbl_format));
}

/*
 * Function table initilaization to invoke the APIs corresponding to different
 * packet technologies as defined in enum gmpls_label_type .
 * The funcation table array element must be NULL if a particular label type is
 * not supported or the label type doesn't support the respective service, in
 * this case free the specified label pool server.
 */
static void (* const gen_lps_free_pf[]) (struct nsm_master *nm,
                                   struct route_node *rn,
                                   enum gmpls_label_type  lbl_format) =
  {
#ifdef HAVE_PACKET
  nsm_lp_server_free_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  };

void
nsm_lp_server_gen_free (struct nsm_master *nm,
                     struct route_node *rn,
                     enum gmpls_label_type  lbl_format)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lps_free_pf[lbl_format]) (nm, rn, lbl_format);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS*/
      default:
        return;
        break;
    }
}

int
nsm_lp_server_pkt_free_block_wrap(struct nsm_master *nm,
                              u_int16_t label_space,
                              u_int32_t block,
                              u_char protocol,
                              u_int8_t service_owner,
                              enum gmpls_label_type  lbl_format)
{
  return (nsm_lp_server_pkt_free_block (nm, label_space, block, protocol,
                                        service_owner, lbl_format));
}

/*
 * Function table initilaization to invoke the APIs corresponding to different
 * packet technologies as defined in enum gmpls_label_type .
 * The funcation table array element must be NULL if a particular label type is
 * not supported or the label type doesn't support the respective service, in
 * this case free the block number specified.
 */
static int (* const gen_lps_free_block_pf[]) (struct nsm_master *nm,
                                     u_int16_t label_space,
                                     u_int32_t block,
                                     u_char protocol,
                                     u_int8_t service_owner,
                                     enum gmpls_label_type  lbl_format) =
  {
#ifdef HAVE_PACKET
  nsm_lp_server_pkt_free_block_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  };

int
nsm_lp_server_gen_free_block (struct nsm_master *nm,
                          u_int16_t label_space,
                          u_int32_t block,
                          u_char protocol,
                          u_int8_t service_owner,
                          enum gmpls_label_type  lbl_format)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lps_free_block_pf[lbl_format]) (nm, label_space, block,
                                         protocol, service_owner, lbl_format);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS*/

      default:
        return LP_SERVER_UNDEFINED_LBL_TYPE;
        break;
    }
}

void
nsm_lp_server_static_block_release_wrap (struct nsm_master *nm,
                                   struct nsm_label_space *nls,
                                   u_char protocol,
                                   enum gmpls_label_type  lbl_format)
{
  return (nsm_lp_server_static_block_release (nm, nls, protocol));
}

/*
 * Function table initilaization to invoke the APIs corresponding to different
 * packet technologies as defined in enum gmpls_label_type .
 * The funcation table array element must be NULL if a particular label type is
 * not supported or the label type doesn't support the respective service, in
 * this case releasing static block from the label pool server.
 */
static void (* const gen_rel_blk_pf[]) (struct nsm_master *nm,
                                  struct nsm_label_space *nls,
                                  u_char protocol,
                                  enum gmpls_label_type  lbl_format) =
  {
#ifdef HAVE_PACKET
  nsm_lp_server_static_block_release_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  };

void
nsm_lp_server_gen_static_block_release (struct nsm_master *nm,
                            struct nsm_label_space *nls,
                            u_char protocol,
                            enum gmpls_label_type  lbl_format)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_rel_blk_pf[lbl_format]) (nm, nls, protocol, lbl_format);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS*/
      default:
        return;
        break;
    }
}

u_int32_t
nsm_lp_server_get_label_value_wrap (struct nsm_master *nm,
                                u_int16_t label_space,
                                int get_maximum,
                                u_int8_t service_owner,
                                enum gmpls_label_type  lbl_format)
{
  return (nsm_lp_server_pkt_get_label_value (nm, label_space, get_maximum,
                                             service_owner, lbl_format));
}


/*
 * Function table initilaization to invoke the APIs corresponding to different
 * packet technologies as defined in enum gmpls_label_type .
 * The funcation table array element must be NULL if a particular label type is
 * not supported or the label type doesn't support the respective service, in
 * this case obtaining the minimum and maximum label values for this server.
 */
static u_int32_t (* const gen_lps_get_max_pf[]) (struct nsm_master *nm,
                                 u_int16_t label_space,
                                 int get_maximum,
                                 u_int8_t service_owner,
                                 enum gmpls_label_type  lbl_format) =
  {
#ifdef HAVE_PACKET
  nsm_lp_server_get_label_value_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  };

u_int32_t
nsm_lp_server_gen_get_label_value (struct nsm_master *nm,
                              u_int16_t label_space,
                              int get_maximum,
                              u_int8_t service_owner,
                              enum gmpls_label_type  lbl_format)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lps_get_max_pf[lbl_format]) (nm, label_space, get_maximum,
                                                  service_owner, lbl_format);

        break;
#endif /* HAVE_PACKET || HAVE_GMPLS*/
      default:
        return LP_SERVER_UNDEFINED_LBL_TYPE;
        break;
    }
}

u_int32_t
nsm_lp_server_get_range_label_value_wrap (struct nsm_master *nm,
                                u_int16_t label_space,
                                int get_maximum,
                                u_int8_t service_owner,
                                enum gmpls_label_type  lbl_format)
{
  return (nsm_lp_server_pkt_get_range_label_value (nm, label_space, get_maximum,
                                                   service_owner, lbl_format));
}


/*
 * Function table initilaization to invoke the APIs corresponding to different
 * packet technologies as defined in enum gmpls_label_type .
 * The funcation table array element must be NULL if a particular label type is
 * not supported or the label type doesn't support the respective service, in
 * this case obtaining the min and max label range values for the server.
 */
static u_int32_t (* const gen_lps_get_rmax_pf[]) (struct nsm_master *nm,
                                 u_int16_t label_space,
                                 int get_maximum,
                                 u_int8_t service_owner,
                                 enum gmpls_label_type  lbl_format) =
  {
#ifdef HAVE_PACKET
  nsm_lp_server_get_range_label_value_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  };

u_int32_t
nsm_lp_server_gen_get_range_label_value (struct nsm_master *nm,
                                    u_int16_t label_space,
                                    int get_maximum,
                                    u_int8_t service_owner,
                                    enum gmpls_label_type  lbl_format)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lps_get_rmax_pf[lbl_format]) (nm, label_space, get_maximum,
                                                  service_owner, lbl_format);

        break;
#endif /* HAVE_PACKET || HAVE_GMPLS*/
      default:
        return LP_SERVER_UNDEFINED_LBL_TYPE;
        break;
    }
}

int
nsm_lp_server_pkt_in_use_wrap(struct nsm_master *nm,
                          u_int32_t label_space,
                          u_int8_t range_owner,
                          enum gmpls_label_type  lbl_format)
{
  return (nsm_lp_server_pkt_in_use (nm, label_space, range_owner, lbl_format));
}

/*
 * Function table initilaization to invoke the APIs corresponding to different
 * packet technologies as defined in enum gmpls_label_type .
 * The funcation table array element must be NULL if a particular label type is
 * not supported or the label type doesn't support the respective service, in
 * this case find out if the lp server corresponding to the interface index
 * passed in is in use. If argument is 0xffffffff, check if ANY labelspaces are
 * in use.
 */
static int (* const gen_lps_in_use_pf[]) (struct nsm_master *nm,
                                  u_int32_t label_space,
                                  u_int8_t range_owner,
                                  enum gmpls_label_type  lbl_format) =
 {
#ifdef HAVE_PACKET
  nsm_lp_server_pkt_in_use_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  };

int
nsm_lp_server_gen_in_use (struct nsm_master *nm,
                        u_int32_t label_space,
                        u_int8_t range_owner,
                        enum gmpls_label_type  lbl_format)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lps_in_use_pf[lbl_format]) (nm, label_space, range_owner,
                                                 lbl_format);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS*/
      default:
        return LP_SERVER_UNDEFINED_LBL_TYPE;
        break;
    }
}

void
nsm_lp_server_clean_for_w_status_wrap(struct nsm_master *nm,
                                 u_int16_t label_space,
                                 u_char status,
                                 u_int8_t service_owner,
                                 enum gmpls_label_type  lbl_format)
{
  return (nsm_lp_server_pkt_clean_for_w_status (nm, label_space, status,
                                                service_owner, lbl_format));
}

/*
 * Function table initilaization to invoke the APIs corresponding to different
 * packet technologies as defined in enum gmpls_label_type .
 * The funcation table array element must be NULL if a particular label type is
 * not supported or the label type doesn't support the respective service, in
 * this case clean up everything pertaining to the passed in protocol that have
 * specified status.
 */
static void (* const gen_lps_clean_wstat_pf[]) (struct nsm_master *nm,
                                    u_int16_t label_space,
                                    u_char status,
                                    u_int8_t service_owner,
                                    enum gmpls_label_type  lbl_format) =
  {
#ifdef HAVE_PACKET
  nsm_lp_server_clean_for_w_status_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  };

void
nsm_lp_server_gen_clean_for_w_status (struct nsm_master *nm,
                                 u_int16_t label_space,
                                 u_char status,
                                 u_int8_t service_owner,
                                 enum gmpls_label_type  lbl_format)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lps_clean_wstat_pf[lbl_format]) (nm, label_space, status,
                                                    lbl_format, service_owner);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS*/
      default:
        return;
        break;
    }
}

void
nsm_lp_server_clean_in_use_for_wrap(struct nsm_master *nm,
                               u_char protocol,
                               u_int8_t range_owner,
                               enum gmpls_label_type  lbl_format)
{
  return (nsm_lp_server_pkt_clean_in_use_for (nm, protocol, range_owner,
                                              lbl_format));
}

/*
 * Function table initilaization to invoke the APIs corresponding to different
 * packet technologies as defined in enum gmpls_label_type .
 * The funcation table array element must be NULL if a particular label type is
 * not supported or the label type doesn't support the respective service, in
 * this case clean up everything pertaining to the passed in protocol that are
 * in use.
 */
static void (* const gen_lps_clean_inuse_pf[]) (struct nsm_master *nm,
                                  u_char protocol,
                                  u_int8_t range_owner,
                                  enum gmpls_label_type  lbl_format) =
  {
#ifdef HAVE_PACKET
  nsm_lp_server_clean_in_use_for_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  };

void
nsm_lp_server_gen_clean_in_use_for (struct nsm_master *nm,
                                u_char protocol,
                                u_int8_t range_owner,
                                enum gmpls_label_type  lbl_format)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lps_clean_inuse_pf[lbl_format]) (nm, protocol, range_owner,
                                                      lbl_format);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS*/
      default:
        return;
        break;
    }
}

void
nsm_lp_server_clean_stale_for_wrap(struct nsm_master *nm,
                               u_char protocol,
                               u_int8_t range_owner,
                               enum gmpls_label_type  lbl_format)
{
  return (nsm_lp_server_pkt_clean_stale_for (nm, protocol, range_owner,
                                             lbl_format));
}

/*
 * Function table initilaization to invoke the APIs corresponding to different
 * packet technologies as defined in enum gmpls_label_type .
 * The funcation table array element must be NULL if a particular label type is
 * not supported or the label type doesn't support the respective service, in
 * this case clean up everything pertaining to the passed in protocol that are
 * stale.
 */
static void (* const gen_lps_clean_stalefor_pf[]) (
                                   struct nsm_master *nm,
                                   u_char protocol,
                                   u_int8_t range_owner,
                                   enum gmpls_label_type  lbl_format) =
  {
#ifdef HAVE_PACKET
  nsm_lp_server_clean_stale_for_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  };

void
nsm_lp_server_gen_clean_stale_for (struct nsm_master *nm,
                              u_char protocol,
                              u_int8_t range_owner,
                              enum gmpls_label_type  lbl_format)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lps_clean_stalefor_pf[lbl_format]) (nm, protocol,
                                                     range_owner, lbl_format);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS*/
      default:
        return;
        break;
    }
}

void
nsm_lp_server_set_stale_for_wrap(struct nsm_master *nm,
                            u_char protocol,
                            u_int8_t range_owner,
                            enum gmpls_label_type  lbl_format)
{
  return (nsm_lp_server_pkt_set_stale_for (nm, protocol, range_owner));
}

/*
 * Function table initilaization to invoke the APIs corresponding to different
 * packet technologies as defined in enum gmpls_label_type .
 * The funcation table array element must be NULL if a particular label type is
 * not supported or the label type doesn't support the respective service, in
 * this case set everything pertaining to the passed in protocol that are
 * stale.
 */
static void (* const gen_lps_set_stalefor_pf[]) (struct nsm_master *nm,
                               u_char protocol,
                               u_int8_t range_owner,
                               enum gmpls_label_type  lbl_format) =
  {
#ifdef HAVE_PACKET
  nsm_lp_server_set_stale_for_wrap,
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  };

void
nsm_lp_server_gen_set_stale_for (struct nsm_master *nm,
                            u_char protocol,
                            u_int8_t range_owner,
                            enum gmpls_label_type  lbl_format)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lps_set_stalefor_pf[lbl_format]) (nm, protocol,
                                                    range_owner, lbl_format);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS*/
      default:
        return;
        break;
    }
}

void
nsm_lp_server_get_stale_for_wrap(struct nsm_master *nm,
                            struct nsm_msg_service *message,
                            u_char protocol,
                            struct nsm_msg_label_pool **label_pools,
                            u_int16_t *label_pool_num,
                            u_int8_t range_owner,
                            enum gmpls_label_type  lbl_format)
{
  return (nsm_lp_server_pkt_get_stale_for (nm, message, protocol, label_pools,
                                           label_pool_num, range_owner,
                                           lbl_format));
}

/*
 * Function table initilaization to invoke the APIs corresponding to different
 * packet technologies as defined in enum gmpls_label_type .
 * The funcation table array element must be NULL if a particular label type is
 * not supported or the label type doesn't support the respective service, in
 * this case get stale entries pertaining to the passed in protocol, construct a list
 *  of "struct nsm_msg_label_pool" type to be returned to the client, and
 * unstale the entries.
 */
static void (* const gen_lps_get_stalefor_pf[]) (struct nsm_master *nm,
                             struct nsm_msg_service *message,
                             u_char protocol,
                             struct nsm_msg_label_pool **label_pools,
                             u_int16_t *label_pool_num,
                             u_int8_t range_owner,
                             enum gmpls_label_type  lbl_format) =
  {
#ifdef HAVE_PACKET
  nsm_lp_server_get_stale_for_wrap,
#endif
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NULL,
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
  };

void
nsm_lp_server_gen_get_stale_for (struct nsm_master *nm,
                                 struct nsm_msg_service *message,
                                 u_char protocol,
                                 struct nsm_msg_label_pool **label_pools,
                                 u_int16_t *label_pool_num,
                                 u_int8_t range_owner,
                                 enum gmpls_label_type  lbl_format)
{
  switch (lbl_format)
    {
#ifdef HAVE_PACKET
      case GMPLS_LABEL_PACKET:
#endif /* HAVE_PACKET */
#if (defined HAVE_PACKET) || (defined HAVE_GMPLS)
        return (*gen_lps_get_stalefor_pf[lbl_format]) (nm, message, protocol,
                                                  label_pools, label_pool_num,
                                                  range_owner, lbl_format);
        break;
#endif /* HAVE_PACKET || HAVE_GMPLS*/

      default:
        return;
        break;
    }
}

