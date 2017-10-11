
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
#include "lp_client.h"
#include "nsmd.h"
#ifdef HAVE_DSTE
#include "dste.h"
#include "nsm_dste.h"
#endif /* HAVE_DSTE */
#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */
#include "nsm_mpls.h"
#include "nsm_interface.h"
#include "nsm_message.h"
#include "nsm_lp_serv_packet.h"

/* Maximum blocks for a server node. */
int
nsm_lp_server_pkt_max_blocks (struct lp_server *server)
{
  int block1;
  int block2;

  if (! server)
    return LABEL_BLOCK_INVALID;

  block1 = (server->min_label_value - 1) / PACKET_LABEL_BUCKET_SIZE;
  block2 = (server->max_label_value - 1) / PACKET_LABEL_BUCKET_SIZE;

  return block2 - block1 + 1;
}

/* Create a new label pool server for a given label space */
struct lp_server*
nsm_lp_server_pkt_new (struct nsm_master *nm,
                     u_int16_t label_space,
                     u_int8_t range_owner,
                     enum gmpls_label_type  lbl_format)
{
  struct lp_server *server;
  struct route_node *rn;
  struct prefix p;
  int block1;
  int block2;
  int i;

  server = XCALLOC (MTYPE_LABEL_POOL_SERVER, sizeof (struct lp_server));
  if (! server)
    return NULL;

  /* Memset prefix to NULL */
  pal_mem_set (&p, 0, sizeof (struct prefix));

  /* Set label space for his server */
  server->label_space = label_space;

  /* Get max and min label values for this server */
  server->min_label_value = nsm_lp_server_pkt_get_label_value (nm, label_space,
                                                   0, range_owner, lbl_format);
  server->max_label_value = nsm_lp_server_pkt_get_label_value (nm, label_space,
                                                   1, range_owner, lbl_format);

  /* Figure out which block the min label lies in. All blocks before
     that are to be marked as used */
  block1 = (server->min_label_value - 1) / PACKET_LABEL_BUCKET_SIZE;
  for (i = 0; i < block1; i++)
    (server->lp_block[i]).status = LP_SERVER_BLOCK_IN_USE;

  /* Figure out which block the max label lies in. All blocks after
     that are to be marked as used */
  block2 = (server->max_label_value - 1) / PACKET_LABEL_BUCKET_SIZE;
  for (i = block2 + 1; i < LABEL_BLOCK_INVALID; i++)
    (server->lp_block[i]).status = LP_SERVER_BLOCK_IN_USE;

  /* Set the total free block num */
  server->total_free_blocks = block2 - block1 + 1;

  /* Set the first free block */
  server->first_free_block = block1;

  /*
   * Now add this server to the label pool server table.
   */

  if (nm->label_pool_table[lbl_format] == NULL)
    {
      /* Create a new route table */
      nm->label_pool_table[lbl_format] = route_table_init ();
      zlog_warn (NSM_ZG, "LPS: Init label pool table for lbl_format %d ls %d "
                          "range_owner %d\n",
                 lbl_format, label_space, range_owner);
    }

  /* Create prefix for the label space */
  p.prefixlen = MAX_LABEL_SPACE_BITLEN;
  p.family = AF_INET;
  PREP_FOR_NETWORK (p.u.prefix4.s_addr, label_space, MAX_LABEL_SPACE_BITLEN);

  /* Get a new node from the route table */
  rn = route_node_get (nm->label_pool_table[lbl_format], (struct prefix *)&p);

  /* Check if this node already had something */
  if (rn->info)
    route_unlock_node (rn);

  /* Now set the label pool server in this node's info */
  rn->info = server;

  /* Also set the back pointer */
  server->rn = rn;

  /* Initialize the range based block allocation data if applicable */
  nsm_lp_server_pkt_init_ranges(nm, label_space, server);

  /* Done */

  return server;
}

void
nsm_lp_server_pkt_init_ranges (struct nsm_master *nm,
                          u_int16_t label_space,
                          struct lp_server *server)
{
  struct nsm_label_space *nls = NULL;
  int index = 0;
  int block1 = 0;
  int block2 = 0;
  int blk_id = 0;

  /* Get the correct label space. */
  nls =  nsm_mpls_label_space_lookup (nm, label_space);

  if (! nls)
    {
      zlog_warn (NSM_ZG, "LPS: Label range init failed for label space %d\n",
                 label_space);
      return;
    }

  /* If ranges are configured, initialize data: first block, total blocks,
   * mark all relevant blocks with range-ownership */

  for (index = 0; index < LABEL_POOL_RANGE_MAX; index++)
    {
      if (!((nls->service_ranges[index]).from_label) &&
          !((nls->service_ranges[index]).to_label))
        {
          continue;
        }

      /* find the blocks in which the label range falls */
      block1 = ((nls->service_ranges[index]).from_label - 1) /
                                              PACKET_LABEL_BUCKET_SIZE;
      if (((nls->service_ranges[index]).from_label - 1) % 1)
        block1++;

      block2 = ((nls->service_ranges[index]).to_label - 1) /
                                              PACKET_LABEL_BUCKET_SIZE;
      if (((nls->service_ranges[index]).to_label - 1) % 1)
        block2++;

      server->first_free_range_block[index] = block1;
      server->total_free_range_blocks[index] = (block2 - block1) + 1;

      /* Mark range-ownership for all the blocks within this range */
      for (blk_id = block1; blk_id <= block2; blk_id++)
        {
          (server->lp_block[blk_id]).range_owner |=
                                   nsm_lp_server_get_service_range_type(index);
#ifdef HAVE_LPS_DEBUG
          zlog_info (NSM_ZG, "LPS: nsm_lp_server_pkt_init_ranges: ls %d "
                              "blk id %d range_owner %d\n",
                  label_space, blk_id, (server->lp_block[blk_id]).range_owner);
#endif
        }
    }
}

/*
 * Get the label pool server specified. If it's not in the
 * table, create it.
 */
struct lp_server *
nsm_lp_server_pkt_get (struct nsm_master *nm, u_int16_t label_space, int create,
                   u_int8_t range_owner, enum gmpls_label_type  lbl_format)
{
  struct prefix p;
  struct route_node *rn;
  struct lp_server *server;

  /* If the table doesnt exist, create it if such is desired */
  if (nm->label_pool_table[lbl_format] == NULL && create)
    {
      nm->label_pool_table[lbl_format] = route_table_init ();
#ifdef HAVE_LPS_DEBUG
      zlog_info (NSM_ZG, "LPS: nsm_lp_server_pkt_get label pool table "
                          "for lbl_format %d ls %d range_owner %d\n",
                 lbl_format, label_space, range_owner);
#endif
    }
  else if ((nm->label_pool_table[lbl_format] == NULL) && (! create))
    return NULL;

  /* Create a prefix for this labelspace */
  p.prefixlen = MAX_LABEL_SPACE_BITLEN;
  p.family = AF_INET;
  PREP_FOR_NETWORK (p.u.prefix4.s_addr, label_space, MAX_LABEL_SPACE_BITLEN);

  server = NULL;
  rn = route_node_lookup (nm->label_pool_table[lbl_format], (struct prefix *)&p);
  if (!rn)
    {
      if (create)
        server = nsm_lp_server_pkt_new (nm, label_space, range_owner,
                                        lbl_format);
      if (! server)
        return NULL;
    }
  else
    {
      /* Unlock node */
      route_unlock_node (rn);

      /* Server set */
      server = rn->info;
    }

  return server;
}

/* Helper function to find the next free block. */
int
nsm_lp_server_pkt_find_next_free (struct lp_server *server)
{
  int i;
  int max_blocks;

  if (server->total_free_blocks == 0)
    {
      server->first_free_block = -1;
      return -1;
    }

  /* Start searching at block after the previously free one. */
  max_blocks = nsm_lp_server_pkt_max_blocks (server);
  for (i = server->first_free_block + 1; i <= max_blocks; i++)
    {
      if ((server->lp_block[i]).status == LP_SERVER_BLOCK_AVAILABLE)
        {
          /* Found a free one. */
          server->first_free_block = i;
          return 0;
        }
    }

  /* If not found, check for an available block. Start from initial
     block and check till the one right before previous first_free_block. */
  for (i = 0; i < server->first_free_block; i++)
    {
      if ((server->lp_block[i]).status == LP_SERVER_BLOCK_AVAILABLE)
        {
          /* Found a free one. */
          server->first_free_block = i;
          return 0;
        }
    }

  /* Nothing found. Set appropriate errors. */
  server->first_free_block = -1;
  return -1;
}

/*
 * The following routine returns the first free block available.
 */
u_int32_t
nsm_lp_server_pkt_get_block (struct nsm_master *nm,
                         u_int16_t label_space,
                         u_char protocol,
                         struct nsm_msg_pkt_block_list *blist,
                         struct lp_serv_lset_block *ls_blk,
                         u_int8_t *range_ret_data,
                         u_int8_t range_owner,
                         enum gmpls_label_type  lbl_format)
{
  struct lp_server *server;
  struct nsm_label_space *nls;
  int next_free;
  int ret;
  int serv_index = -1;

  /* Check if label space exists. */
  nls =  nsm_mpls_label_space_lookup (nm, label_space);
  if (! nls)
    {
      zlog_warn (NSM_ZG, "LPS: Label block request from client %s failed "
                 "because no interface has been initialized in the NSM "
                 "with label space %d\n", modname_strl (protocol),
                 label_space);
      return LABEL_BLOCK_INVALID;
    }

  /* Get the server for this label space */
  server = nsm_lp_server_pkt_get (nm, label_space, 1 /* create */,
                                  range_owner, lbl_format);

  /* Check for service owner range validity  */
  serv_index = nsm_lp_server_get_service_range_index(range_owner);

  /* Check for label set requests first with the presence of block list(s)
   */
  if (blist)
    {
      if (blist->blk_req_type == BLK_REQ_TYPE_GET_LS_BLOCK)
        {
          return (nsm_lp_server_pkt_process_lset_req (nm, label_space,
                                                  server, blist,
                                                  ls_blk, range_owner,
                                                   serv_index, protocol));
        }
    }
  /* Call range based allocation if requested by the caller
   */
  else if ((serv_index != LABEL_POOL_SERVICE_ERROR) &&
           (range_owner != LABEL_POOL_SERVICE_UNSUPPORTED))
    {
      if ((CHECK_FLAG (nls->config, NSM_LABEL_SPACE_CONFIG_MIN_LABEL_RANGE)) &&
          (CHECK_FLAG (nls->config, NSM_LABEL_SPACE_CONFIG_MAX_LABEL_RANGE)))
      {
        return (nsm_lp_server_pkt_get_range_based_block(nm, label_space,
                                   server, range_owner, serv_index, protocol,
                                   range_ret_data));
      }
    }

  /* Get the first free block for this server for flat label space */
  ret = server->first_free_block;
  if (ret == -1)
    {
      zlog_warn (NSM_ZG, "LPS: No more label blocks available for lspace %d\n",
                 label_space);
      return LABEL_BLOCK_INVALID;
    }

  if ((server->lp_block[ret]).status != LP_SERVER_BLOCK_AVAILABLE)
    {
      /* Code is broken somewhere */
#ifdef HAVE_LPS_DEBUG
      zlog_info (NSM_ZG, "LPS: Label pool block mismatch");
#endif
      return LABEL_BLOCK_INVALID;
    }

  /* Mark block as used */
  (server->lp_block[ret]).status = LP_SERVER_BLOCK_IN_USE;

  /* Set protocol */
  (server->lp_block[ret]).protocol = protocol;

  /* Decrement the counter */
  server->total_free_blocks = server->total_free_blocks - 1;

  /* Now move the free pointer to the next one */
  next_free = nsm_lp_server_pkt_find_next_free (server);
  if (next_free < 0)
    {
#ifdef HAVE_LPS_DEBUG
      zlog_info (NSM_ZG, "LPS: Label block %d was the last available block "
                 "for labelspace %d", ret, label_space);
#endif
    }

#ifdef HAVE_LPS_DEBUG
  zlog_info (NSM_ZG, "LPS: Label block %d is allocated for "
             "for labelspace %d", ret, label_space);
#endif

  return ret;
}

int
nsm_lp_server_pkt_process_lset_req (struct nsm_master *nm,
                                  u_int16_t label_space,
                                  struct lp_server *server,
                                  struct nsm_msg_pkt_block_list *blist,
                                  struct lp_serv_lset_block *ls_blk,
                                  u_int8_t service_owner,
                                  int service_index,
                                  u_char protocol)
{
  u_int32_t ret_lbl = LABEL_VALUE_INVALID;

  /* Service module related range checks should be done in the client before
   * dispatching this request to server
   */
  switch (blist->blk_req_type)
    {
      case BLK_REQ_TYPE_GET_LS_BLOCK:
        {
          return (nsm_lp_server_pkt_get_lset_block (nm, label_space, server,
                                blist, ls_blk, service_owner, service_index,
                                &ret_lbl, protocol));
          break;
        }

      default:
        return LABEL_BLOCK_INVALID;
    }

}

int
nsm_lp_server_pkt_get_lset_block (struct nsm_master *nm,
                                u_int16_t label_space,
                                struct lp_server *server,
                                struct nsm_msg_pkt_block_list *blist,
                                struct lp_serv_lset_block *ls_blk,
                                u_int8_t service_owner,
                                int service_index,
                                u_int32_t *ret_lbl,
                                u_char protocol)
{
  u_int32_t btype;
  u_int32_t list_length = 0;
  u_int32_t idx = 0;
  u_int32_t blk_id = 0;
  int found = -1;
  int next_free;

  if ((service_owner) &&
      (server->total_free_range_blocks[service_index] == -1))
    {
      zlog_warn (NSM_ZG, "LPS: No more label blocks available for "
                          "label range %d\n", service_owner);
      return LABEL_BLOCK_INVALID;
    }
  else if (server->total_free_blocks == -1)
    {
      zlog_warn (NSM_ZG, "LPS: No more label blocks available for "
                          "label range %d\n", service_owner);
      return LABEL_BLOCK_INVALID;
    }

  /* First try to find an unused block within the inclusive block list(index 0)
   * Otherwise, try to locate a block within the exclude list; note that though
   * this is exclude block list, client sent only valid blocks that can be
   * used after ignoring the exclusion list/range (index 1)
   */
  for (btype = 0; ((btype < LSO_BLK_TYPE_MAX) && (found == -1)); btype++)
    {
      list_length = blist[btype].blk_count;
      for (idx = 0; idx < list_length; idx++)
        {
          blk_id = blist[btype].lset_blocks[idx];
          if (server->lp_block[blk_id].status == LP_SERVER_BLOCK_IN_USE)
            {
              continue;
            }

          if (service_owner)
            {
              if ((server->lp_block[blk_id].range_owner & service_owner) &&
                  (server->lp_block[blk_id].status == LP_SERVER_BLOCK_AVAILABLE))
                {
                  found = btype;
                  break;
                }
            }
          else if (server->lp_block[blk_id].status == LP_SERVER_BLOCK_AVAILABLE)
            {
              found = btype;
              break;
            }
        }
    }

  if (found == -1)
    {
      return LP_SERVER_LSET_BLOCK_NOT_FOUND;
    }

  /* Mark block as used */
  (server->lp_block[blk_id]).status = LP_SERVER_BLOCK_IN_USE;

  /* Set protocol */
  (server->lp_block[blk_id]).protocol = protocol;

  if (service_owner)
    {
      next_free = nsm_lp_server_pkt_range_find_next_free(nm, label_space,
                                        server, service_owner, service_index);
      if (next_free < 0)
        {
#ifdef HAVE_HAVE_LPS_DEBUG
          zlog_info (NSM_ZG, "LPS: Label block %d was the last available block "
                     "for labelspace %d", label_space);
#endif
        }
    }
  else
    {
      /* Decrement the counter */
      server->total_free_blocks = server->total_free_blocks - 1;

      /* Now move the free pointer to the next one */
      next_free = nsm_lp_server_pkt_find_next_free (server);
      if (next_free < 0)
        {
#ifdef HAVE_LPS_DEBUG
          zlog_info (NSM_ZG, "LPS: Label block %d was the last available block "
                     "for labelspace %d", label_space);
#endif
        }
    }

  /* Return the block found and relevant data for client to process */
  ls_blk->lset_blk_type = found;
  ls_blk->block_id = blk_id;

  return LP_SERVER_LSET_BLOCK_FOUND;
}

int
nsm_lp_server_pkt_range_find_next_free(struct nsm_master *nm,
                                 u_int16_t label_space,
                                 struct lp_server *server,
                                 u_int8_t service_owner,
                                 int service_index)
{
  u_int32_t min_blks = 0;
  u_int32_t max_blks = 0;
  int blk = 0;
  struct nsm_label_space *nls = NULL;

  /* Get the correct label space. */
  nls =  nsm_mpls_label_space_lookup (nm, label_space);

  /* Now move the free pointer to the next one in the range if avaialble
   * and required */
  if (server->total_free_range_blocks[service_index] == 0)
    {
      server->first_free_range_block[service_index] = -1;
#ifdef HAVE_LPS_DEBUG
      zlog_info (NSM_ZG, "LPS: Last label pool block allocated for "
                         " the range: %d \n", service_owner);
#endif
    }
  else
    {
      if (nls)
        {
          min_blks = (nls->service_ranges[service_index].from_label - 1) /
            PACKET_LABEL_BUCKET_SIZE;
          max_blks = (nls->service_ranges[service_index].to_label - 1) /
            PACKET_LABEL_BUCKET_SIZE;
        }
      /* Blocks may be allocated in random order */
      for (blk = server->first_free_range_block[service_index] + 1;
           blk < max_blks; blk++)
        {
          if ((server->lp_block[blk].range_owner & service_owner) &&
              (server->lp_block[blk].status == LP_SERVER_BLOCK_AVAILABLE))
            {
              server->first_free_range_block[service_index] = blk;
              return 0;
            }
        }
      /* If not found look for one at the start of the range */
      for (blk = min_blks;
           blk < server->first_free_range_block[service_index]; blk++)
        {
          if ((server->lp_block[blk].range_owner & service_owner) &&
              (server->lp_block[blk].status == LP_SERVER_BLOCK_AVAILABLE))
            {
              server->first_free_range_block[service_index] = blk;
              return 0;
            }
        }
    }
    /* None exist */
    server->first_free_range_block[service_index] = -1;

    return -1;
}

u_int32_t
nsm_lp_server_pkt_get_range_based_block (struct nsm_master *nm,
                                    u_int16_t label_space,
                                    struct lp_server *server,
                                    u_int8_t service_owner,
                                    int service_index,
                                    u_char protocol,
                                    u_int8_t *range_ret_data)
{
  int ret = LABEL_BLOCK_INVALID;
  int rc = LP_SERVER_FAILURE;

  /* Get the first free block for this range */
  ret = server->first_free_range_block[service_index];
  if (ret == -1)
    {
      zlog_warn (NSM_ZG, "LPS: No more label blocks available for "
                 "label range %d\n", service_owner);
      return LABEL_BLOCK_INVALID;
    }

  if ((server->lp_block[ret]).status != LP_SERVER_BLOCK_AVAILABLE)
    {
      /* Code is broken somewhere */
      zlog_err (NSM_ZG, "LPS: Label pool range block %d not available", ret);
      return LABEL_BLOCK_INVALID;
    }

  /* Ensure the block belongs to this range owner*/
  if (!((server->lp_block[ret]).range_owner & service_owner))
  {
    /* Something is wrong in the code flow */
    zlog_err (NSM_ZG, "LPS: Label pool block mismatch for the range: %d \n",
                        service_owner);
    return LABEL_BLOCK_INVALID;
  }

  /* Mark block as used */
  (server->lp_block[ret]).status = LP_SERVER_BLOCK_IN_USE;

  /* Set protocol */
  (server->lp_block[ret]).protocol = protocol;

  /* Decrement the counter */
  if (service_owner)
    {
      server->total_free_range_blocks[service_index] -= 1;
    }

  /* Now move the free pointer to the next one in the range if avaialble */
  if (server->total_free_range_blocks[service_index] == 0)
    {
      server->first_free_range_block[service_index] -= 1;
#ifdef HAVE_LPS_DEBUG
      zlog_info (NSM_ZG, "LPS: Last label pool block allocated for the range: %d \n",
                          service_owner);
#endif
    }
  else
    {
      rc = nsm_lp_server_pkt_range_find_next_free(nm, label_space,
                                        server, service_owner, service_index);
      if (rc < 0)
        {
#ifdef HAVE_LPS_DEBUG
          zlog_info (NSM_ZG, "LPS: Label block %d was the last available block "
                     "for labelspace %d", ret, label_space);
#endif
        }
    }
  *range_ret_data = LABEL_RANGE_BLOCK_ALLOC_SUCCESS;


  zlog_info (NSM_ZG, "LPS: Range label block %d is allocated for for "
                     "labelspace %d", ret, label_space);

  return ret;
}

/* Free the specific label pool server */
void
nsm_lp_server_pkt_free (struct nsm_master *nm, struct route_node *rn,
                     enum gmpls_label_type  lbl_format)
{
  struct lp_server *server;
  struct route_node *rn1;

  /* Get the lp server from this node */
  server = rn->info;

  /* Free the server */
  XFREE (MTYPE_LABEL_POOL_SERVER, server);

  /* Remove node. */
  rn->info = NULL;
  route_unlock_node (rn);

  /* If there arent any nodes in this table, delete it */
  rn1 = route_top (nm->label_pool_table[lbl_format]);
  if (rn1 == NULL)
    {
      route_table_finish (nm->label_pool_table[lbl_format]);
      nm->label_pool_table[lbl_format] = NULL;
    }
  else
    route_unlock_node (rn1);

  /* Done */
}

/*
 * Free the block number specified.
 */
int
nsm_lp_server_pkt_free_block (struct nsm_master *nm,
                         u_int16_t label_space,
                         u_int32_t block,
                         u_char protocol,
                         u_int8_t range_owner,
                         enum gmpls_label_type  lbl_format)
{
  struct lp_server *server;

  server = nsm_lp_server_pkt_get (nm, label_space, 0 /* dont create */,
                                  range_owner, lbl_format);
  if (! server)
    {
      zlog_err (NSM_ZG, "LPS: Server for label space %d does not exist",
                 label_space);
      return -1;
    }
  else if ((server->lp_block[block]).protocol == protocol)
    {
      /* Unset block attributes. */
      (server->lp_block[block]).status = LP_SERVER_BLOCK_AVAILABLE;
      (server->lp_block[block]).protocol = APN_PROTO_UNSPEC;

      /* Check if this is a service-owner range based deletion request
       * and aslo that the request is no from legacy API.
       * Range ownership stays intact during block free by the client */
      if ((range_owner != LABEL_POOL_SERVICE_UNSUPPORTED) &&
          (server->lp_block[block]).range_owner & range_owner)
        {
          return (nsm_lp_server_pkt_free_range_based_block(server, range_owner,
                                                           block));
      }

      /* Increment the counter */
      server->total_free_blocks = server->total_free_blocks + 1;

      /* Now check if the lp_first_free_block value needs to be changed */
      if ((server->first_free_block > block) ||
          (server->first_free_block == -1))
        server->first_free_block = block;
      else if (server->first_free_block == block)
        {
          zlog_err (NSM_ZG, "LPS: Block to be freed was marked as free");
          return -1;
        }

      /* Check if all the blocks in this labelspace specific
         server are empty. If yes, free this server */
      if (server->total_free_blocks == nsm_lp_server_pkt_max_blocks (server))
        /* None of the blocks are in use */
        nsm_lp_server_pkt_free (nm, server->rn, lbl_format);
    }

  return 0;
}

int
nsm_lp_server_pkt_free_range_based_block(struct lp_server *server,
                                    u_int8_t range_owner,
                                    u_int32_t block)
{
  int service_index = -1;

  service_index = nsm_lp_server_get_service_range_index(range_owner);

  if (service_index == LABEL_POOL_SERVICE_ERROR)
    {
      zlog_warn (NSM_ZG, "LPS: Invalid range owner : %d", range_owner);
      return LP_SERVER_INVALID_RANGE_INDEX;
    }

  /* Increment the number of blocks for the current range */
  server->total_free_range_blocks[service_index] =
                          (server->total_free_range_blocks[service_index]) + 1;

  /* Now check if the first free block for range needs to be changed */
  if ((server->first_free_range_block[service_index] > block) ||
      (server->first_free_range_block[service_index] == -1))
    {
      server->first_free_range_block[service_index] = block;
    }
  else if (server->first_free_range_block[service_index] == block)
    {
      zlog_err (NSM_ZG, "LPS: Range-block to be freed was marked as free");
      return LP_SERVER_RANGE_BLOCK_FREE_ERROR;
    }

  return 0;
}

/* Figure out the minimum and maximum label values for this server. */
u_int32_t
nsm_lp_server_pkt_get_label_value (struct nsm_master *nm,
                              u_int16_t label_space,
                              int get_maximum,
                              u_int8_t range_owner,
                              enum gmpls_label_type  lbl_format)
{
  struct nsm_label_space *nls;

  /* Get the correct label space. */
  nls =  nsm_mpls_label_space_lookup (nm, label_space);
  if (! nls)
    return LABEL_VALUE_INVALID;

  if (! get_maximum)
    {
      return nls->min_label_val;
    }
  else
    {
      return nls->max_label_val;
    }
}

/* Function to figure out the minimum and maximum label values for the range */
u_int32_t
nsm_lp_server_pkt_get_range_label_value (struct nsm_master *nm,
                                    u_int16_t label_space,
                                    int get_maximum,
                                    u_int8_t range_owner,
                                    enum gmpls_label_type  lbl_format)
{
  struct lp_server *server = NULL;
  struct nsm_label_space *nls = NULL;
  int service_index = -1;

  if (range_owner == LABEL_POOL_SERVICE_UNSUPPORTED)
    {
      return LABEL_VALUE_INVALID;
    }

  /* Get the correct label space. */
  nls =  nsm_mpls_label_space_lookup (nm, label_space);
  if (! nls)
    return LABEL_VALUE_INVALID;

  server = nsm_lp_server_pkt_get (nm, label_space, 0 /* dont create */,
                                  range_owner, lbl_format);
  if (! server)
    {
      zlog_err (NSM_ZG, "LPS: Server for label space %d does not exist",
                 label_space);
      return LABEL_VALUE_INVALID;
    }

  service_index = nsm_lp_server_get_service_range_index(range_owner);

  if (service_index == LABEL_POOL_SERVICE_ERROR)
    {
      zlog_warn (NSM_ZG, "LPS: Invalid range owner %d\n", range_owner);
      return LABEL_VALUE_INVALID;
    }

  if (! get_maximum)
    {
      return nls->service_ranges[service_index].from_label;
    }
  else
    {
      return nls->service_ranges[service_index].to_label;
    }
}

/* Figure out if the lp server corresponding to the interface index
   passed in is in use. If argument is 0xffffffff, check if ANY labelspaces
   are in use. */
int
nsm_lp_server_pkt_in_use (struct nsm_master *nm, u_int32_t label_space,
                      u_int8_t range_owner,
                      enum gmpls_label_type  lbl_format)
{
  struct lp_server *server = NULL;

  /* Special handling for 0xffffffff. If there are ANY servers defined,
     return TRUE. */
  if (label_space == 0xffffffff && nm->label_pool_table[lbl_format])
    return 1;

  server = nsm_lp_server_pkt_get (nm, label_space, 0 /* dont create */,
                                  range_owner, lbl_format);
  if (server)
    /* Server found for this label space. Return TRUE. */
    return 1;

  return 0;
}


/* Clean up everything pertaining to the passed in protocol that have specified status. */
void
nsm_lp_server_pkt_clean_for_w_status (struct nsm_master *nm,
                                 u_char protocol,
                                 u_char status,
                                 u_int8_t range_owner,
                                 enum gmpls_label_type  lbl_format)
{
  struct route_node *rn;
  struct lp_server *server;
  int found;
  int max_blocks;
  int i;
  int service_id = -1;

  /* If the label pool table doesnt exist, just get out */
  if (! nm->label_pool_table[lbl_format])
    return;

  if (range_owner != LABEL_POOL_SERVICE_UNSUPPORTED)
    {
      service_id = nsm_lp_server_get_service_range_index(range_owner);

      if (service_id == LABEL_POOL_SERVICE_ERROR)
        {
          /* Error in the code flow */
          zlog_warn (NSM_ZG, "LPS: Invalid service id %d\n", range_owner);
          return;
        }
    }

  for ( rn = route_top (nm->label_pool_table[lbl_format]); rn;
                                                        rn = route_next (rn))
    {
      /* Get the lp server */
      server = rn->info;

      /* Only move ahead for a valid server */
      if (! server)
        continue;

      found = 0;
      max_blocks = nsm_lp_server_pkt_max_blocks (server);

      for (i = 0; i <= max_blocks; i++)
        {
          if (((server->lp_block[i]).protocol == protocol) &&
              ((server->lp_block[i]).status == status))
            {
              /* Found a block for this protocol */
              if (! found)
                {
                  /* Reset the first free block if required */
                  if ((server->first_free_block > i) ||
                      (server->first_free_block == -1))
                    {
                      server->first_free_block = i;
                    }

                  if ((range_owner != LABEL_POOL_SERVICE_UNSUPPORTED) &&
                      (server->lp_block[i]).range_owner & range_owner)
                    {
                      if ((server->first_free_range_block[service_id] > i) ||
                          (server->first_free_range_block[service_id] == -1))
                        {
                          server->first_free_range_block[service_id] = i;
                        }
                    }
                  found = 1;
                }

              /* Reset all values in it */
              pal_mem_set (&(server->lp_block[i]), 0, sizeof (struct lp_block));
            }
        }
    }
}
/* Clean up everything pertaining to the passed in protocol that are in use. */
void
nsm_lp_server_pkt_clean_in_use_for (struct nsm_master *nm,
                              u_char protocol,
                              u_int8_t range_owner,
                              enum gmpls_label_type  lbl_format)
{
    nsm_lp_server_pkt_clean_for_w_status(nm, protocol, LP_SERVER_BLOCK_IN_USE,
                                         range_owner, lbl_format);
}

/* Clean up everything pertaining to the passed in protocol that are stale. */
void
nsm_lp_server_pkt_clean_stale_for (struct nsm_master *nm,
                             u_char protocol,
                             u_int8_t range_owner,
                             enum gmpls_label_type  lbl_format)
{
    nsm_lp_server_pkt_clean_for_w_status(nm, protocol, LP_SERVER_BLOCK_STALE,
                                         range_owner, lbl_format);
}

/* Mark entries pertaining to the passed in protocol with stale flag. */
void
nsm_lp_server_pkt_set_stale_for (struct nsm_master *nm, u_char protocol,
                           enum gmpls_label_type  lbl_format)
{
  struct route_node *rn;
  struct lp_server *server;
  int max_blocks;
  int i;

  /* If the label pool table doesnt exist, just get out */
  if (! nm->label_pool_table[lbl_format])
    return;

  for ( rn = route_top (nm->label_pool_table[lbl_format]); rn;
        rn = route_next (rn))
    {
      /* Get the lp server */
      server = rn->info;

      /* Only move ahead for a valid server */
      if (! server)
        continue;

      max_blocks = nsm_lp_server_pkt_max_blocks (server);
      for (i = 0; i <= max_blocks; i++)
        {
          if ((server->lp_block[i]).protocol == protocol)
            /* Found a block for this protocol */
            server->lp_block[i].status = LP_SERVER_BLOCK_STALE;
        }
    }
}

/* Get stale entries pertaining to the passed in protocol, construct a list
   of "struct nsm_msg_label_pool" type to be returned to the client, and
   unstale the entries. */
void
nsm_lp_server_pkt_get_stale_for (struct nsm_master *nm,
                             struct nsm_msg_service *message,
                             u_char protocol,
                             struct nsm_msg_label_pool **label_pools,
                             u_int16_t *label_pool_num,
                             u_int8_t range_owner,
                             enum gmpls_label_type  lbl_format)
{
  struct route_node *rn;
  struct lp_server *server;
  int max_blocks;
  int i;
  u_int16_t count = 0;

  *label_pools = NULL;
  *label_pool_num = 0;

  /* If the label pool table doesnt exist, just get out */
  if (! nm->label_pool_table[lbl_format])
    return;

  /* TODO: We walk through the table and the blocks once to count the
     number of matched label pools, and then walk through the same
     again to create the required data. Consider doing this in one
     walk through */
  for (rn = route_top (nm->label_pool_table[lbl_format]); rn;
       rn = route_next (rn))
    {
      /* Get the lp server */
      server = rn->info;

      /* Only move ahead for a valid server */
      if (! server)
        continue;

      max_blocks = nsm_lp_server_pkt_max_blocks (server);
      for (i = 0; i <= max_blocks; i++)
        {
          if (((server->lp_block[i]).protocol == protocol) &&
              ((server->lp_block[i]).status == LP_SERVER_BLOCK_STALE))
            count++;
        }
    }

  if (! count)
    return;

  *label_pools = XCALLOC (MTYPE_NSM_MSG_LABEL_POOL,
                          sizeof (struct nsm_msg_label_pool) * count);
  if (! *label_pools)
    return;

  NSM_SET_CTYPE (message->cindex, NSM_SERVICE_CTYPE_LABEL_POOLS);
  *label_pool_num = count;
  count = 0;

  for (rn = route_top (nm->label_pool_table[lbl_format]); rn;
       rn = route_next (rn))
    {
      /* Get the lp server */
      server = rn->info;

      /* Only move ahead for a valid server */
      if (! server)
        continue;

      max_blocks = nsm_lp_server_pkt_max_blocks (server);
      for (i = 0; i <= max_blocks; i++)
        {
          if (((server->lp_block[i]).protocol == protocol) &&
              ((server->lp_block[i]).status == LP_SERVER_BLOCK_STALE))
            {
              (*label_pools)[count].label_space = server->label_space;
              (*label_pools)[count].label_block = i;
              (*label_pools)[count].min_label =
                 nsm_lp_server_pkt_get_label_value (nm, server->label_space, 0,
                                                    range_owner, lbl_format);
              (*label_pools)[count].max_label =
                 nsm_lp_server_pkt_get_label_value (nm, server->label_space, 1,
                                                    range_owner, lbl_format);

              /* No need to set the cindex since we won't be using them */
/*              NSM_SET_CTYPE ((*label_pools)[count].cindex,      */
/*                             NSM_LABEL_POOL_CTYPE_LABEL_BLOCK); */
/*              NSM_SET_CTYPE ((*label_pools)[count].cindex,      */
/*                             NSM_LABEL_POOL_CTYPE_LABEL_RANGE); */
              server->lp_block[i].status = LP_SERVER_BLOCK_IN_USE;
              count++;
            }
        }
    }
}

