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
#include "lp_client_packet.h"

/* Create a new label pool client */
struct lp_client *
lp_client_pkt_new (struct lib_globals *zg, struct route_table *table,
               u_int16_t label_space, u_int32_t min_label,
               u_int32_t max_label, enum label_pool_id lp_module)
{
  struct lp_client *client;
  struct route_node *rn;
  struct prefix p;
  int ctr;

#ifdef HAVE_LPC_DEBUG
  zlog_info (zg, "LPC-S: lp_client_pkt_new table 0x%x ls %d "
                 " min %d max %d service %d",
                 table, label_space, min_label, max_label, lp_module);
#endif

  client = XCALLOC (MTYPE_LABEL_POOL_CLIENT, sizeof (struct lp_client));
  if (! client)
    return NULL;

  /* pal_mem_set prefix to NULL */
  pal_mem_set (&p, 0, sizeof (struct prefix));

  /* Set the labelspace for this client */
  client->label_space = label_space;

  /* Create an empty list for available blocks;
   * set up enough block lists of the total number of services that a module
   * can request, as in LP_SERV_TYPE_MAX */
  for (ctr = 0; ctr < LP_SERV_TYPE_MAX; ctr++)
    {
      client->block_list[ctr] = bitmap_passive_new (LABEL_BUCKET_SIZE,
                                                    min_label, max_label);
      if (! client->block_list[ctr])
        {
          goto cleanup;
        }
    }

  /*
   * Add this client to the label pool client table
   */

  /* Create prefix for the label space */
  p.prefixlen = MAX_LABEL_SPACE_BITLEN;
  p.family = AF_INET;
  PREP_FOR_NETWORK (p.u.prefix4.s_addr, label_space, MAX_LABEL_SPACE_BITLEN);

  /* Get a new node from the route table */
  rn = route_node_get (table, (struct prefix *)&p);

  /* Check if this node already had something */
  if (rn->info)
    {
      route_unlock_node (rn);
      goto cleanup;
    }

  /* Now set the label pool client in this node's info */
  rn->info = client;

  /* Also set the back pointer */
  client->rn = rn;

  /* Set refcount to 1. */
  client->refcount = 1;

  /* Set default for the service in use */
  client->service_in_use |= lp_module;

#ifdef HAVE_LPC_DEBUG
  zlog_info (zg, "LPC-E: lp_client_pkt_new client 0x%x ", client);
#endif

  return client;

cleanup:
  for (ctr = 0; ctr < LP_SERV_TYPE_MAX; ctr++)
    {
      if (client->block_list[ctr])
        {
          XFREE (MTYPE_BITMAP, client->block_list[ctr]);
          client->block_list[ctr] = NULL;
        }
    }
  XFREE (MTYPE_LABEL_POOL_CLIENT, client);
  return NULL;
}

/* Process a bind request's reply.  */
int
lp_client_pkt_bind_block (struct lib_globals *zg, struct route_table *table,
                      u_int16_t label_space, u_int32_t block,
                      u_int32_t min_label, u_int32_t max_label,
                      enum label_pool_id lp_module)
{
  struct lp_client *client;
  int serv_index;

#ifdef HAVE_LPC_DEBUG
  zlog_info (zg, "LPC-S: lp_client_pkt_bind_block table 0x%x ls %d blk %d "
                 " min %d max %d service %d",
                table, label_space, block, min_label, max_label, lp_module);
#endif

  if (block >= LABEL_BLOCK_INVALID)
    {
      /* Something went wrong on the NSM end */
      zlog_err (zg, "LPC: NSM has no more blocks left for label space %d",
                label_space);
      return LP_CLIENT_FAILURE;
    }

  /* Get the client */
  client = lp_client_lookup (zg, table, label_space);
  if (! client)
    {
      client = lp_client_pkt_new (zg, table, label_space, min_label, max_label,
                                  lp_module);
      if (! client)
        return LP_CLIENT_FAILURE;
    }

  serv_index = lp_client_get_service_index (lp_module);

  if (serv_index == LABEL_POOL_SERVICE_ERROR)
    {
       zlog_err (zg, "LPC: Invalid service type for block allocation for "
                     "ls %d, service %d", label_space, lp_module);
       return LP_CLIENT_FAILURE;
    }

  /* Add a new node to the list. */
  bitmap_add_new_node (client->block_list[serv_index], block,
                                                        BITMAP_NODE_ADD_NONE);

#ifdef HAVE_LPC_DEBUG
  zlog_info (zg, "LPC-E: New blk add:blk-id %d ls %d min %d max %d service %d",
                block, label_space, min_label, max_label, lp_module);
#endif

  return LP_CLIENT_SUCCESS;
}

/* Process a release request's reply. */
int
lp_client_pkt_release_block (struct lib_globals *zg, struct route_table *table,
                         u_int16_t label_space, u_int32_t block,
                         enum label_pool_id serv_module)
{
  struct bitmap_block *node;
  struct lp_client *client;
  int serv_index;

#ifdef HAVE_LPC_DEBUG
  zlog_info (zg, "LPC-S: lp_client_pkt_release_block table 0x%x ls %d "
                 "blk %d service %d",
                 table, label_space, block, serv_module);
#endif

  client = lp_client_lookup (zg, table, label_space);
  if (client == NULL)
    return LP_CLIENT_FAILURE;

  serv_index = lp_client_get_service_index (serv_module);
  if (serv_index == LABEL_POOL_SERVICE_ERROR)
    {
       zlog_err (zg, "LPC: Invalid service type for block allocation "
                     "for ls %d, service %d",
                     label_space, serv_module);
       return LP_CLIENT_FAILURE;
    }
  /* Get node and free it. */
  node = bitmap_block_get_by_id (client->block_list[serv_index], block);
  if (node)
    bitmap_block_free (node);

#ifdef HAVE_LPC_DEBUG
  zlog_info (zg, "LPC-E: lp_client_pkt_release_block succ table 0x%x ls %d "
                 "blk %d service %d",
                 table, label_space, block, serv_module);
#endif

  return LP_CLIENT_SUCCESS;
}

/* Send label block request message to NSM. */
int
lp_client_pkt_request_block_send (struct lib_globals *zg,
                            struct route_table *table,
                            u_int16_t label_space,
                            struct nsm_client *nc,
                            struct nsm_msg_pkt_block_list *blist,
                            enum label_pool_id lp_module)
{
  int ret;
  struct nsm_msg_label_pool msg;
  struct nsm_msg_label_pool reply;

#ifdef HAVE_LPC_DEBUG
  zlog_info (zg, "LPC-S: lp_client_pkt_request_block_send table 0x%x ls %d "
                 "nc 0x%x blist 0x%x service %d",
                 table, label_space, nc, blist, lp_module);
#endif

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_label_pool));
  pal_mem_set (&reply, 0, sizeof (struct nsm_msg_label_pool));
  /* BGP request platform wide label space. */
  msg.label_space = label_space;

  /* Get individaul block */
  if (! blist)
    {
      /* Range owner to get a  block returned within the service range */
      if (lp_module != LABEL_POOL_SERVICE_UNSUPPORTED)
        {
          NSM_SET_CTYPE (msg.cindex, NSM_LABEL_POOL_CTYPE_LABEL_RANGE);
          msg.range_owner |= lp_module;
        }
    }

  /* Label Set if present */
  if (blist)
    {
      NSM_SET_CTYPE (msg.cindex, NSM_LABEL_POOL_CTYPE_BLOCK_LIST);
      msg.blk_list[INCL_BLK_TYPE] = blist[INCL_BLK_TYPE];
      msg.blk_list[EXCL_BLK_TYPE] = blist[EXCL_BLK_TYPE];
    }

  ret = nsm_client_send_label_request (nc, &msg, &reply);
  if (ret < 0)
    {
      zlog_err (zg, "LPC: lp_client_pkt_request_block_send ret %d ", ret);
      return ret;
    }

  if (blist && NSM_CHECK_CTYPE (reply.cindex, NSM_LABEL_POOL_CTYPE_BLOCK_LIST))
    {
      if (reply.blk_list[INCL_BLK_TYPE].blk_req_type ==
                                                   BLK_REQ_TYPE_GET_LS_BLOCK)
        {
          blist[INCL_BLK_TYPE] = reply.blk_list[INCL_BLK_TYPE];
        }
      if (reply.blk_list[EXCL_BLK_TYPE].blk_req_type ==
                                                   BLK_REQ_TYPE_GET_LS_BLOCK)
        {
          blist[EXCL_BLK_TYPE] = reply.blk_list[EXCL_BLK_TYPE];
        }
      ret = LP_CLIENT_SUCCESS;
    }
  if ((NSM_CHECK_CTYPE (reply.cindex, NSM_LABEL_POOL_CTYPE_LABEL_BLOCK)) ||
      (NSM_CHECK_CTYPE (reply.cindex, NSM_LABEL_POOL_CTYPE_LABEL_RANGE)))
    {
      /* Assigning the label block to table. */
      ret = lp_client_pkt_bind_block (zg, table, reply.label_space,
                                       reply.label_block, reply.min_label,
                                       reply.max_label, lp_module);
    }

#ifdef HAVE_LPC_DEBUG
  zlog_info (zg, "LPC-E lp_client_pkt_request_block_send reply ls %d "
                 "blk %d min %d max %d service %d",
                 reply.label_space, reply.label_block, reply.min_label,
                 reply.max_label);
#endif

  return (ret);
}

/* Send label block release message to NSM. */
int
lp_client_pkt_release_block_send (struct lib_globals *zg,
                              struct route_table *table,
                              u_int16_t label_space,
                              u_int32_t block,
                              struct nsm_client *nc,
                              int nsm_cleanup,
                              enum label_pool_id serv_module)
{
  int ret;
  struct nsm_msg_label_pool msg;

#ifdef HAVE_LPC_DEBUG
  zlog_info (zg, "LPC-S: lp_client_pkt_release_block_send table 0x%x ls %d "
                 "blk %d nc 0x%x nsm_clnup %d service %d",
                 table, label_space, block, nc, nsm_cleanup, serv_module);
#endif

  /* Intimate NSM for label removal only if nsm_cleanup is TRUE */
  if (nsm_cleanup)
    {

     /* BGP request platform wide label space.  */
     pal_mem_set (&msg, 0, sizeof (struct nsm_msg_label_pool));
     msg.label_space = label_space;
     NSM_SET_CTYPE (msg.cindex, NSM_LABEL_POOL_CTYPE_LABEL_BLOCK);
     msg.label_block = block;

     /* Send the range owner so that the block is deleted from the respective
      * service range */
     if (lp_client_is_service_type_valid(serv_module))
       {
         msg.range_owner |= serv_module;
       }
     else
       {
         zlog_info(zg, "LPC: invalid range owner for release_block_send");
       }

     ret = nsm_client_send_label_release (nc, &msg);
     if (ret < 0)
      {
        zlog_err (zg, "LPC: label release send failed");
        return ret;
      }
    }

  /* Free the label block from the table.  */
  lp_client_pkt_release_block (zg, table, label_space, block, serv_module);

#ifdef HAVE_LPC_DEBUG
  zlog_info(zg, "LPC-E: label release send success");
#endif

  return LP_CLIENT_SUCCESS;
}

/* Get the label pool client specified. If it's not in the table,
   create it.  */
struct lp_client *
lp_client_pkt_lookup (struct lib_globals *zg, struct route_table *table,
                  u_int16_t label_space, enum label_pool_id serv_type)
{
  struct prefix p;
  struct route_node *rn;

  /* If no table, get out */
  if (table == NULL)
    return NULL;

  /* Create a prefix for this labelspace */
  p.prefixlen = MAX_LABEL_SPACE_BITLEN;
  p.family = AF_INET;
  PREP_FOR_NETWORK (p.u.prefix4.s_addr, label_space, MAX_LABEL_SPACE_BITLEN);

  rn = route_node_lookup (table, (struct prefix *)&p);
  if (rn)
    {
      route_unlock_node (rn);
      return (struct lp_client *)rn->info;
    }

  return NULL;
}

/* API to be used by protocols.  Request the first block during
   initialization of an interface on the protocol end.  */
int
lp_client_pkt_init_label_block (struct lib_globals *zg,
                         struct route_table *table,
                         u_int16_t label_space,
                         struct nsm_client *nc,
                         enum label_pool_id serv_module)
{
  struct lp_client *client = NULL;
  int ret;
  int serv_index;

#ifdef HAVE_LPC_DEBUG
  zlog_info (zg, "LPC-S: lp_client_pkt_init_label_block table 0x%x ls %d "
                 "nc 0x%x service %d",
                 table, label_space, nc, serv_module);
#endif

  serv_index = lp_client_get_service_index (serv_module);
  if (serv_index == LABEL_POOL_SERVICE_ERROR)
    {
       zlog_err (zg, "LPC: Invalid service type : ls %d, service %d",
                label_space, serv_module);
       return LP_CLIENT_FAILURE;
    }

  client = lp_client_lookup (zg, table, label_space);
  if (! client ||
      ! client->block_list[serv_index] ||
      ! client->block_list[serv_index]->curr_node)
    {
      /*
       * No client exists, which means that no other interface
       * has asked for a block for this given label space.
       *
       * We are now free to request a block.
       */
      ret = lp_client_request_block_send (zg, table, label_space, nc);
      if (ret < 0)
        {
          zlog_err (zg, "LPC: Could not get label block for label space %d "
                    "from NSM", label_space);
          return ret;
        }
    }
  else
    client->refcount++;

#ifdef HAVE_LPC_DEBUG
  zlog_info (zg, "LPC-E: lp_client_pkt_init_label_block client 0x%x", client);
#endif

  return 0;
}

void
lp_client_pkt_init (struct lib_globals *zg, struct route_table **table)
{
  if (! *table)
    /* Now create the table to hold lp_client structs */
    *table = route_table_init ();
}

int
lp_client_pkt_set_service_type(struct lib_globals *zg,
                      struct route_table *table,
                      u_int16_t label_space,
                      enum label_pool_id serv_module)
{
  struct lp_client *client = NULL;

  if (serv_module < LABEL_POOL_SERVICE_RSVP ||
      serv_module > LABEL_POOL_RANGE_MAX)
    {
      return LP_CLIENT_INVALID_SERVICE_TYPE;
    }

  client = lp_client_lookup (zg, table, label_space);

  if (!client)
    {
      return LP_CLIENT_DOESNT_EXIST;
    }

  client->service_in_use |= serv_module;

  return LP_CLIENT_SUCCESS;
}

void
lp_client_pkt_block_list_free (struct lib_globals *zg,
                        struct route_table *table,
                        struct lp_client *client,
                        struct nsm_client *nc,
                        enum label_pool_id serv_module)
{
  struct bitmap_block *node, *node_next;
  struct nsm_msg_label_pool msg;
  int ret;
  int i;
  int serv_index;

  serv_index = lp_client_get_service_index (serv_module);
  if (serv_index == LABEL_POOL_SERVICE_ERROR)
    {
       zlog_err (zg, "LPC: Invalid service type service %d", serv_module);
       return;
    }

  if (! client->block_list[serv_index])
    return;

  /* BGP request platform wide label space.  */
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_label_pool));
  NSM_SET_CTYPE (msg.cindex, NSM_LABEL_POOL_CTYPE_LABEL_BLOCK);
  msg.label_space = client->label_space;

  for (i = 0; i < BITMAP_HASH_SIZE; i++)
    {
      for (node = client->block_list[serv_index]->hash[i]; node;
           node = node_next)
        {
          node_next = node->next;

          /* Send release to NSM. */
          msg.label_block = node->id;

          /* If client is going for a graceful restart do not
           * release the label blocks to the pool */
#ifdef HAVE_RESTART
          if (! FLAG_ISSET (zg->flags,
                            LIB_FLAG_GRACEFUL_SHUTDOWN_IN_PROGRESS))
#endif /* HAVE_RESTART */
            {
              ret = nsm_client_send_label_release (nc, &msg);
              if (ret < 0)
                {
                  zlog_warn (zg, "LPC: block release %d failed", node->id);
                }
            }

          /* Free node. */
          bitmap_block_free (node);
          client->block_list[serv_index]->hash[i] = NULL;
        }
    }

  /* Now free the block list. */
  bitmap_free (client->block_list[serv_index]);
  client->block_list[serv_index] = NULL;
}

/* Free label pool.  */
void
lp_client_pkt_free (struct lib_globals *zg, struct route_table *table,
                struct nsm_client *nc, enum label_pool_id serv_module)
{
  struct route_node *rn;
  struct lp_client *client;

  if (! table)
    return;

  /* Walk through all of client block.  */
  for (rn = route_top (table); rn; rn = route_next (rn))
    if ((client = rn->info) != NULL)
      {
        /* Delete the block list. */
        lp_client_pkt_block_list_free (zg, table, client, nc, serv_module);

        /* Now free the client */
        XFREE (MTYPE_LABEL_POOL_CLIENT, client);
        rn->info = NULL;
        route_unlock_node (rn);
      }

  /* Delete the table itself */
  route_table_finish (table);
}

/* Free label set internally used memory */
void
lp_client_pkt_lset_free (struct lib_globals *zg,
                    struct route_table *table,
                    struct nsm_client *nc,
                    struct lp_client_pkt_lso_list *lsol,
                    struct lp_client_pkt_lso_blk_list *pblkl,
                    enum label_pool_id serv_module)
{
  u_int32_t type;
  struct lp_client_pkt_lso_list *also = NULL;
  struct lp_client_pkt_lso *lso_node = NULL;
  struct lp_client_pkt_lso_blk_list *apblk = NULL;
  struct lp_client_pkt_lso_blk *blk_node;

  for (type = ACTION_TYPE_MIN; type < ACTION_TYPE_MAX; type++)
    {
      also = &lsol[type];
      if (also->count > 0)
        {
          while ((lso_node = also->head) != NULL)
            {
              /* Free/delete  packet label set object data, node */
              if (lso_node->pkt_labels)
                {
                  XFREE (MTYPE_LABEL_SET_NODE, lso_node->pkt_labels);
                  lso_node->pkt_labels = NULL;
                }
              LP_CLIENT_PKT_LSO_DEL (also, lso_node);
              XFREE (MTYPE_LABEL_SET_NODE, lso_node);
              lso_node = NULL;
            }
        }
    }

  for (type = LSO_BLK_TYPE_MIN; type < LSO_BLK_TYPE_MAX; type++)
    {
      apblk = &pblkl[type];
      if (apblk->count > 0)
        {
          while ((blk_node = apblk->head) != NULL)
            {
              /* Delete packet label set object data node */
              LP_CLIENT_PKT_LSO_DEL (apblk, blk_node);
              XFREE (MTYPE_LABEL_SET_NODE, blk_node);
              blk_node = NULL;
            }
        }
    }
}

/* Get the total number of labels in the list, excluding the block
   corresponding to the block id passed in.  */
u_int32_t
lp_client_pkt_get_available_label_count (struct lib_globals *zg,
                                 struct route_table *table,
                                 u_int16_t label_space,
                                 u_int32_t block_id,
                                 enum label_pool_id serv_module)
{
  struct lp_client *client;
  int serv_index;

  client = lp_client_lookup (zg, table, label_space);
  if (client == NULL)
    return 0;

  serv_index = lp_client_get_service_index (serv_module);
  if (serv_index == LABEL_POOL_SERVICE_ERROR)
    {
       zlog_err (zg, "LPC: Invalid service type service %d", serv_module);
       return 0;
    }
  return bitmap_get_available_index_count (client->block_list[serv_index],
                                           block_id);
}

/* Check if there is atleast one label available. */
int
lp_client_pkt_is_usable (struct lib_globals *zg,
                    struct route_table *table,
                    u_int16_t label_space,
                    u_int32_t block_id,
                    enum label_pool_id serv_module)
{
  struct lp_client *client = lp_client_lookup (zg, table, label_space);
  int serv_index;
  int rc = PAL_FALSE;

  serv_index = lp_client_get_service_index (serv_module);
  if (serv_index == LABEL_POOL_SERVICE_ERROR)
    {
       zlog_err (zg, "LPC: Invalid service type service %d rc=%d (0:F)",
                     serv_module, rc);
       return rc;
    }

  if (client)
    rc = bitmap_is_usable (client->block_list[serv_index], block_id);

#ifdef HAVE_LPC_DEBUG
  zlog_info (zg, "LPC: lp_client_pkt_is_usable rc=%d (0:F)", rc);
#endif
  return rc;
}

/* Helper function to update the request failure counter.  */
void
lp_client_pkt_update_request_failure (struct lib_globals *zg,
                              struct route_table *table,
                              struct lp_client *client,
                              u_char type,
                              enum label_pool_id serv_module)
{
  if (type == REQUEST_FAILED)
    client->req_fail_cnt = client->req_fail_cnt + 1;
  else if (type == RELEASE)
    client->rel_fail_cnt = client->rel_fail_cnt + 1;
}

struct bitmap_block *
lp_client_pkt_get_block (struct lib_globals *zg,
                    struct route_table *table,
                    u_int16_t label_space,
                    u_int32_t label,
                    u_int32_t *req_fail_cnt,
                    enum label_pool_id serv_module)
{
  struct lp_client *client;
  int serv_index;

  /* Get the client associated with this label space */
  client = lp_client_lookup (zg, table, label_space);
  if (! client)
    return NULL;

  serv_index = lp_client_get_service_index (serv_module);
  if (serv_index == LABEL_POOL_SERVICE_ERROR)
    {
       zlog_err (zg, "LPC: Invalid service type service %d", serv_module);
       return PAL_FALSE;
    }

  /* Update the available label counter. */
  *req_fail_cnt = client->req_fail_cnt;

  /* Get bitmap node. */
  return bitmap_block_get (client->block_list[serv_index], label);
}

/* Request a label.  */
int
lp_client_pkt_request_label (struct lib_globals *zg,
                       struct route_table *table,
                       u_int16_t label_space,
                       struct nsm_client *nc,
                       enum label_pool_id serv_module)
{
  struct lp_client *client;
  u_int32_t label = LABEL_VALUE_INVALID;
  int ret;
  int serv_index;

#ifdef HAVE_LPC_DEBUG
  zlog_info (zg, "LPC-S: lp_client_pkt_request_label table 0x%x ls %d "
                 " nc 0x%x service %d",
                table, label_space, nc, serv_module);
#endif

  /* Get valid client. */
  client = lp_client_lookup (zg, table, label_space);
  if (! client)
    return LABEL_VALUE_INVALID;

  serv_index = lp_client_get_service_index (serv_module);
  if (serv_index == LABEL_POOL_SERVICE_ERROR)
    {
       zlog_err (zg, "LPC: Invalid service type service %d", serv_module);
       return LABEL_VALUE_INVALID;
    }

  /* Get index. */
  ret = bitmap_request_index (client->block_list[serv_index], &label);
  if (ret == BITMAP_FAILURE)
    {
      zlog_err (zg, "LPC: Label req failed for labelspace %d.", label_space);
      lp_client_pkt_update_request_failure (zg, table, client, REQUEST_FAILED,
                                            serv_module);
      return LABEL_VALUE_INVALID;
    }
  else if (ret == BITMAP_REQUEST_NEW_ID)
    {
      ret = lp_client_pkt_request_block_send (zg, table, label_space, nc,
                                               NULL, serv_module);
      if (ret < 0)
        {
          zlog_warn (zg, "LPC: Request for label block failed");

          /* An error return is required here? We don't want to return a label
           * when the block allocation itself failed? */
          return LABEL_VALUE_INVALID;
        }
    }

#ifdef HAVE_LPC_DEBUG
  zlog_info (zg, "LPC-E: Allocated label %d for labelspace %d min %d max %d",
               label, label_space, (client->block_list[serv_index])->min_index,
               (client->block_list[serv_index])->max_index);
#endif

  return label;
}

/* Release a label.  */
int
lp_client_pkt_release_label (struct lib_globals *zg,
                         struct route_table *table,
                         u_int16_t label_space,
                         u_int32_t label,
                         struct nsm_client *nc,
                         enum label_pool_id serv_type)
{
  return lp_client_release_label_cleanup (zg, table, label_space, label, nc,
                                          PAL_TRUE);
}

int
lp_client_pkt_release_label_cleanup (struct lib_globals *zg,
                               struct route_table *table,
                               u_int16_t label_space,
                               u_int32_t label,
                               struct nsm_client *nc,
                               int nsm_cleanup,
                               enum label_pool_id serv_module)
{
  struct lp_client *client;
  u_int32_t block_id;
  int ret;
  int serv_index;

  /* Make sure this label is not outside the allowed range */
  if (label > LABEL_VALUE_MAX || label < LABEL_VALUE_INITIAL)
    {
      zlog_err (zg, "LPC: Label to be released %d is invalid\n", label);
      return -1;
    }


  /* Get client. */
  client = lp_client_lookup (zg, table, label_space);
  if (! client)
    {
      zlog_warn (zg, "LPC: Client not found for label space %d\n", label_space);
      return -1;
    }

  serv_index = lp_client_get_service_index (serv_module);
  if (serv_index == LABEL_POOL_SERVICE_ERROR)
    {
       zlog_err (zg, "LPC: Invalid service type service %d", serv_module);
       return LP_CLIENT_FAILURE;
    }

  /* Release index. */
  ret = bitmap_passive_release_index (client->block_list[serv_index], label,
                                      &block_id);
  if (ret < 0)
    {
      switch (ret)
        {
        case BITMAP_FAILURE:
          zlog_warn (zg, "LPC: Label pool client: Release failed");
          break;
        case BITMAP_NOT_FOUND:
          zlog_warn (zg, "LPC: Label pool client: No matching node found "
                     "for label space %d", label_space);
          break;
        case BITMAP_INTERNAL_ERROR:
          zlog_warn (zg, "LPC: Label pool client: Internal error");
          break;
        case BITMAP_RE_RELEASE_RCVD:
          zlog_warn (zg, "LPC: Label pool client: Label being released "
                     "was not in use");
          break;
        }

      return ret;
    }

  /* Update release-after-failure counter if required. */
  if (client->req_fail_cnt > 0)
    lp_client_pkt_update_request_failure (zg, table, client, RELEASE,
                                          serv_module);

  /* Release block if required. */
  if (block_id != 0xffffffff)
    {
      ret = lp_client_pkt_release_block_send (zg, table, label_space, block_id,
                                              nc, nsm_cleanup, serv_module);
      if (ret < 0)
        {
          zlog_warn (zg, "LPC: Release of label block failed");
          return ret;
        }
    }

#ifdef HAVE_LPC_DEBUG
  zlog_info (zg, "LPC: Released label %d for labelspace %d min %d max %d",
                 label, label_space, (client->block_list[serv_index])->min_index,
                 (client->block_list[serv_index])->max_index);
#endif

  return 0;
}

/* Release label pool client. */
void
lp_client_pkt_deinit (struct lib_globals *zg, struct route_table *table,
                 u_int16_t label_space, struct nsm_client *nc,
                 enum label_pool_id serv_module)
{
  struct lp_client *client;
  struct route_node *rn;

  /* Get client for label-space. */
  client = lp_client_lookup (zg, table, label_space);
  if (client)
    {
      /* Decrement refcount. */
      if (client->refcount > 0)
        client->refcount--;

      if (client->refcount == 0)
        {
          /* Delete the block list. */
          lp_client_pkt_block_list_free (zg, table, client, nc, serv_module);

          /* Now free the client. */
          rn = client->rn;
          XFREE (MTYPE_LABEL_POOL_CLIENT, client);
          if (rn)
            {
              rn->info = NULL;
              route_unlock_node (rn);
            }
        }
    }
}

int
lp_client_pkt_preproc_lso_range (struct lib_globals *zg,
                           struct lp_client *client,
                           struct lp_client_glabel_list *glabels,
                           struct lp_client_pkt_lso_list *plso,
                           enum label_pool_id serv_module)
{
  struct lp_client_pkt_lso *node;
  u_int32_t* range;
  int rc = LP_CLIENT_SUCCESS;

  range = XCALLOC(MTYPE_LABEL_SET_NODE, sizeof (u_int32_t) * 2);

  if (! range)
    {
      return LP_CLIENT_MEMORY_ALLOC_ERROR;
    }

  rc = (lp_client_pkt_lset_get_range_intersection (client, glabels, &range[0],
                                               &range[1], serv_module));

  if (rc != LP_CLIENT_SUCCESS)
    {
      XFREE (MTYPE_LABEL_SET_NODE, range);
      return rc;
    }

  node = XCALLOC(MTYPE_LABEL_SET_NODE, sizeof (struct lp_client_pkt_lso));
  if (! node)
    {
      XFREE (MTYPE_LABEL_SET_NODE, range);
      return LP_CLIENT_MEMORY_ALLOC_ERROR;
    }

  node->pkt_label_count = 2;
  node->pkt_labels = range;

  LP_CLIENT_PKT_LSO_ADD_TAIL (plso, node);

  return rc;
}

int
lp_client_pkt_preproc_lso_list (struct lib_globals *zg,
                         struct lp_client *client,
                         struct lp_client_glabel_list *glabels,
                         struct lp_client_pkt_lso_list *plso,
                         enum label_pool_id serv_module)
{
  struct lp_client_pkt_lso *node;
  u_int32_t *pkt_lbls = NULL;
  int ctr = 0;
  int index = 0;
  u_int32_t min_val = 0;
  u_int32_t max_val = 0;
  u_int32_t list_len = glabels->glabel_count;
  int start = 0;
  int end = list_len - 1;
  u_int32_t actual_len = list_len;
  u_int32_t pkt_lbl_list[list_len];

  if (!list_len)
    {
      return LP_CLIENT_LBL_SET_ERROR;
    }

  for (ctr = 0; ctr < list_len; ctr++)
    {
      pkt_lbl_list[ctr] = glabels->glabels[ctr].u.mpls_label;
    }

  if (list_len > 1)
  {
    pal_qsort (pkt_lbl_list, list_len, sizeof (u_int32_t),
               lp_client_pkt_lbl_set_compare);
  }

  /* Ensure that the first and last elements are valid labels */
  if ((pkt_lbl_list[start] < LABEL_VALUE_INITIAL) ||
      (pkt_lbl_list[end] < LABEL_VALUE_MAX))
    {
      return LP_CLIENT_LBL_SET_INVALID_LABELS;
    }

  /* Determine the intersecting label list with local label space */
  min_val = client->block_list[serv_module]->min_index;
  max_val = client->block_list[serv_module]->max_index;

  /* Boundary conditions */
  if ((pkt_lbl_list[start] > max_val) ||
      (pkt_lbl_list[end] < min_val))
    {
      zlog_warn (zg, "LPC: Label set preproc: entire list out of label space "
                     " bounds. List start %d, end %d, ls-min %d, ls_max %d",
                     pkt_lbl_list[start], pkt_lbl_list[end],
                     min_val, max_val);
      return LP_CLIENT_LBL_SET_OUT_OF_BOUND_LIST;
    }

  /* One end of the list is outside of the label space */
  if ((pkt_lbl_list[start] > min_val) ||
      (pkt_lbl_list[end] > max_val))
    {
      if (pkt_lbl_list[start] > min_val)
        {
          while ((start <= end) && (pkt_lbl_list[start] > min_val))
            start++;
        }
      if (pkt_lbl_list[end] > max_val)
        {
          while ((end >= start) && (pkt_lbl_list[end] > max_val))
            end--;
        }

      if (start >= end)
        {
          zlog_warn (zg, "LPC: Label set preproc: input list out of bounds "
                         " List start %d, end %d, ls-min %d, ls_max %d",
                         pkt_lbl_list[start], pkt_lbl_list[end],
                         min_val, max_val);
          return LP_CLIENT_LBL_SET_OUT_OF_BOUND_LIST;
        }
    }

  /* Find the sub list that fits within the min, max label values */
  while (start <= end)
    {
      if ((pkt_lbl_list[start] <  min_val) && (start < end))
        {
          start++;
        }
      if ((pkt_lbl_list[end] > max_val) && (end > start))
        {
          end--;
        }
    }

  if ((pkt_lbl_list[start] < min_val) ||
      (pkt_lbl_list[end] > max_val))
    {
      zlog_warn (zg, "LPC: Label set preproc: input list doesnt fit LS "
                     " List start %d, end %d, ls-min %d, ls_max %d",
                     pkt_lbl_list[start], pkt_lbl_list[end],
                     min_val, max_val);
      return LP_CLIENT_LBL_SET_LIST_DOESNT_FIT;
    }

  actual_len = end - start + 1;

#ifdef HAVE_LPC_DEBUG
  zlog_info (zg, "LPC: Label set preproc: list length that fits LS : %d "
                 " List start %d, end %d, ls-min %d, ls_max %d",
                 actual_len, pkt_lbl_list[start], pkt_lbl_list[end],
                 min_val, max_val);
#endif

  pkt_lbls = XCALLOC(MTYPE_LABEL_SET_NODE, sizeof (u_int32_t) * actual_len);

  if (! pkt_lbls)
    {
      return LP_CLIENT_MEMORY_ALLOC_ERROR;
    }

  for (ctr = start; ctr <= end; ctr++)
    {
      pkt_lbls[index++] = pkt_lbl_list[ctr];

#ifdef HAVE_LPC_DEBUG
      zlog_info (zg, "LPC: Label set preproc: list element: idx: %d, val: %d ",
                     ctr, pkt_lbl_list[ctr]);
#endif
    }

  node = XCALLOC(MTYPE_LABEL_SET_NODE, sizeof (struct lp_client_pkt_lso));

  if (! node)
    {
      XFREE (MTYPE_LABEL_SET_NODE, pkt_lbls);
      return LP_CLIENT_MEMORY_ALLOC_ERROR;
    }

  node->pkt_label_count = actual_len;
  node->pkt_labels = pkt_lbls;

  LP_CLIENT_PKT_LSO_ADD_TAIL (plso, node);

  return LP_CLIENT_SUCCESS;

}

/* The process starts off by scanning the first four LS-objects.
 * If either IR or IL exist:
 * a) if both IR and IL exists, find intersection (with IL sorted) or
 * b) if only one of IR or IL exist, start with IR first if it exists else IL
 * c) Find a label from a or b and check against ER/EL (if exists) to
 *    determine if the selected label meets the LS-objects ATs.
 *
 * If neither IR or IL exist:
 * d) check if the curr-blk next label is OK with ER/EL present
 * e) NO: else, find ER/EL block range and find a label beyond these ranges
 * f) else, find a label outside of ER but not in EL
 *
 * If a label can't be located in client scope, send to server a list of
 * IR-blks and/or ER-blks determined in setps a) thru f)
 * g) Server returns a block to client within IR/IL with AT.
 * h) if no IR/IL, server finds a block not in ER/EL and returns it
 * i) if only blocks available are in ER/EL then server returns one or more
 *    blocks within ER/El assuming there is atleast one un-used label that
 *    can be allocated.
 */
int
lp_client_pkt_get_label_from_label_set (struct lib_globals *zg,
                                struct route_table *table,
                                u_int16_t lbl_space,
                                struct nsm_client *nc,
                                struct lp_client_label_set *label_set,
                                struct generalized_label *glabel_ret,
                                enum label_pool_id service_req)
{
  struct lp_client_pkt_lso_list plso_list[ACTION_TYPE_MAX];
  struct lp_client_pkt_lso_blk_list pblk_list[LSO_BLK_TYPE_MAX];
  struct lp_client *client;
  u_int8_t lso_cnt= 0;
  u_int32_t iter = 0;
  u_int32_t type = 0;
  int ret = LP_CLIENT_LBL_SET_ERROR;
  u_int32_t ret_lbl = LABEL_VALUE_INVALID;
  u_int8_t action_type = label_set->lbl_set_obj->action_type;

  glabel_ret->gmpls_label_type = GMPLS_LABEL_PACKET;

  /* Get valid client. */
  client = lp_client_lookup (zg, table, lbl_space);
  if (! client)
    {
      return LABEL_VALUE_INVALID;
    }

  for (type = ACTION_TYPE_MIN; type < ACTION_TYPE_MAX; type++)
    {
      plso_list[type].count = 0;
      plso_list[type].head  = NULL;
      plso_list[type].tail  = NULL;
    }

  for (type = LSO_BLK_TYPE_MIN; type < LSO_BLK_TYPE_MAX; type++)
    {
      pblk_list[type].action_type = 0;
      pblk_list[type].count = 0;
      pblk_list[type].head = NULL;
      pblk_list[type].tail = NULL;
    }

  /* The intent is to run through all the LSOs; if the look out for a label to
   * be allocated needs to be limited to some specific number (less than
   * incoming LSOs) the the variable below shall get that number.
   */
  lso_cnt = label_set->ls_obj_count;

  for (iter = 0; iter < lso_cnt; iter++)
    {
      if (CHECK_FLAG (action_type,  ACTION_TYPE_INCLUDE_RANGE))
        {
          SET_FLAG (pblk_list[INCL_BLK_TYPE].action_type,
                                                  ACTION_TYPE_INCLUDE_RANGE);
          ret = lp_client_pkt_preproc_lso_range (zg, client,
                                      &label_set->lbl_set_obj->glabel_list,
                                      &(plso_list[INCL_RANGE_INDEX]),
                                      service_req);

          if (ret < 0)
            {
              lp_client_pkt_lset_free (zg, table, nc, plso_list, pblk_list,
                                       service_req);
              return ret;
            }
          break;
        }
      else if (CHECK_FLAG (action_type,  ACTION_TYPE_INCLUDE_LIST))
        {
          SET_FLAG (pblk_list[INCL_BLK_TYPE].action_type,
                                                    ACTION_TYPE_INCLUDE_LIST);
          ret = lp_client_pkt_preproc_lso_list (zg, client,
                                      &label_set->lbl_set_obj->glabel_list,
                                      &(plso_list[INCL_LIST_INDEX]),
                                      service_req);

          if (ret < 0)
            {
              lp_client_pkt_lset_free (zg, table, nc, plso_list, pblk_list,
                                       service_req);
              return ret;
            }
          break;
        }
      else if (CHECK_FLAG (action_type,  ACTION_TYPE_EXCLUDE_RANGE))
        {
          SET_FLAG (pblk_list[INCL_BLK_TYPE].action_type,
                                                  ACTION_TYPE_EXCLUDE_RANGE);
          ret = lp_client_pkt_preproc_lso_range (zg, client,
                                      &label_set->lbl_set_obj->glabel_list,
                                      &(plso_list[EXCL_RANGE_INDEX]),
                                      service_req);

          if (ret < 0)
            {
              lp_client_pkt_lset_free (zg, table, nc, plso_list, pblk_list,
                                       service_req);
              return ret;
            }
          break;
        }
      else if (CHECK_FLAG (action_type,  ACTION_TYPE_EXCLUDE_LIST))
        {
          SET_FLAG (pblk_list[INCL_BLK_TYPE].action_type,
                                                  ACTION_TYPE_EXCLUDE_LIST);
          ret = lp_client_pkt_preproc_lso_list (zg, client,
                                      &label_set->lbl_set_obj->glabel_list,
                                      &(plso_list[EXCL_LIST_INDEX]),
                                      service_req);

          if (ret < 0)
            {
              lp_client_pkt_lset_free (zg, table, nc, plso_list, pblk_list,
                                       service_req);
              return ret;
            }
          break;
        }
      else
        {
          zlog_err (zg, "LPC: Invalid action type: %d ", action_type);
          glabel_ret->gmpls_label_type = GMPLS_LABEL_ERROR;
          glabel_ret->u.mpls_label = LABEL_VALUE_INVALID;

          return ret;
        }
    }

  /* To find a label the following order is used: IR, IL, ER, EL */

  /* Try to locate a valid label from the inclusive range */
  iter = plso_list[INCL_RANGE_INDEX].count;
  while (iter)
    {
      ret = lp_client_pkt_process_include_range (zg, client,
                                                 plso_list,
                                                 &(pblk_list[INCL_BLK_TYPE]),
                                                 &ret_lbl,
                                                 service_req);
      if (ret == LP_CLIENT_PKT_LBL_MATCH_FOUND)
        {
          glabel_ret->u.mpls_label = ret_lbl;
          lp_client_pkt_lset_free (zg, table, nc, plso_list, pblk_list,
                                   service_req);
          return ret;
        }
      iter--;
    }

  /* Attempt to locate a valid label from the inclusive list */
  iter = plso_list[INCL_LIST_INDEX].count;
  while (iter)
    {
      ret = lp_client_pkt_process_include_list (zg, client, plso_list,
                                                &(pblk_list[INCL_BLK_TYPE]),
                                                &ret_lbl, service_req);
      if (ret == LP_CLIENT_PKT_LBL_MATCH_FOUND)
        {
          glabel_ret->u.mpls_label = ret_lbl;
          lp_client_pkt_lset_free (zg, table, nc, plso_list, pblk_list,
                                   service_req);
          return ret;
        }
      iter--;
    }

  /* No success with include range/list; try to get one using exclude range */
  iter = plso_list[EXCL_RANGE_INDEX].count;
  while (iter)
    {
      ret = lp_client_pkt_process_exclude_range (zg, client, plso_list,
                                                 &(pblk_list[EXCL_BLK_TYPE]),
                                                 &ret_lbl, service_req);
      if (ret == LP_CLIENT_PKT_LBL_MATCH_FOUND)
        {
          glabel_ret->u.mpls_label = ret_lbl;
          lp_client_pkt_lset_free (zg, table, nc, plso_list, pblk_list,
                                   service_req);
          return ret;
        }
      iter--;
    }

  /* Try to get a label using exclude list */
  iter = plso_list[EXCL_LIST_INDEX].count;
  while (iter)
    {
      ret = lp_client_pkt_process_exclude_list (zg, client,
                                                plso_list,
                                                 &(pblk_list[EXCL_BLK_TYPE]),
                                                 &ret_lbl, service_req);
      if (ret == LP_CLIENT_PKT_LBL_MATCH_FOUND)
        {
          glabel_ret->u.mpls_label = ret_lbl;
          lp_client_pkt_lset_free (zg, table, nc, plso_list, pblk_list,
                                   service_req);
          return ret;
        }
      iter--;
    }

  /* If we are here means that at the lp client scope a label could not be
   * allocated; it is time to query the lp server; check with lp_server
   * for a label block
   */
  if ((pblk_list[INCL_BLK_TYPE].count > 0) ||
      (pblk_list[EXCL_BLK_TYPE].count > 0))
    {
      ret = lp_client_pkt_req_on_demand_block_send (zg, table, lbl_space,
                                           nc, plso_list, pblk_list, glabel_ret,
                                           BLK_REQ_TYPE_NONE, service_req);
    }

  /* Cleanup internally created data if any before exiting this request */
  lp_client_pkt_lset_free (zg, table, nc, plso_list, pblk_list,
                           service_req);
  return ret;

}

int
lp_client_pkt_req_on_demand_block_send (struct lib_globals *zg,
                                struct route_table *table,
                                u_int16_t lbl_space,
                                struct nsm_client *nc,
                                struct lp_client_pkt_lso_list *plsol,
                                struct lp_client_pkt_lso_blk_list *pblk_list,
                                struct generalized_label *glabel_ret,
                                u_int8_t label_req_type,
                                enum label_pool_id service_req)
{
  struct lp_client *client;
  struct nsm_msg_label_pool msg_lset;
  u_int32_t ctr = 0;
  int ret = LP_CLIENT_LBL_SET_ERROR;
  struct   lp_client_pkt_lso_blk *node;
  u_int32_t *incl_blk_list = NULL;
  u_int32_t *excl_blk_list = NULL;

  pal_mem_set (&msg_lset, 0, sizeof (struct nsm_msg_label_pool));

  /* Get valid client. */
  client = lp_client_lookup (zg, table, lbl_space);
  if (! client)
    {
      return LABEL_VALUE_INVALID;
    }

  /* for inclusive, exclusive block lists - sort the block lists */
  if (pblk_list[INCL_BLK_TYPE].count)
    {
      incl_blk_list = XCALLOC(MTYPE_LABEL_SET_NODE, sizeof (u_int32_t) *
                                          pblk_list[INCL_BLK_TYPE].count);

      if (! incl_blk_list)
        {
          return LP_CLIENT_MEMORY_ALLOC_ERROR;
        }

      for (node = pblk_list[INCL_BLK_TYPE].head; node; node = node->next)
        {
          incl_blk_list[ctr] = node->pkt_block;
          ctr++;
        }
      if (CHECK_FLAG (pblk_list[INCL_BLK_TYPE].action_type,
                                                  ACTION_TYPE_INCLUDE_LIST))
        {
          if (pblk_list[INCL_BLK_TYPE].count > 1)
          {
            pal_qsort (incl_blk_list,
                       pblk_list[INCL_BLK_TYPE].count,
                       sizeof (u_int32_t),
                       lp_client_pkt_lbl_set_compare);
          }
        }
    }

  if (pblk_list[EXCL_BLK_TYPE].count)
    {
      ctr = 0;
      excl_blk_list = XCALLOC(MTYPE_LABEL_SET_NODE, sizeof (u_int32_t) *
                                          pblk_list[EXCL_BLK_TYPE].count);
      if (! excl_blk_list)
        {
          XFREE (MTYPE_LABEL_SET_NODE, incl_blk_list);
          return LP_CLIENT_MEMORY_ALLOC_ERROR;
        }

      for (node = pblk_list[EXCL_BLK_TYPE].head; node; node = node->next)
        {
          excl_blk_list[ctr] = node->pkt_block;
          ctr++;
        }
      if (CHECK_FLAG (pblk_list[EXCL_BLK_TYPE].action_type,
                                                  ACTION_TYPE_INCLUDE_LIST))
        {
          if (pblk_list[EXCL_BLK_TYPE].count > 1)
          {
            pal_qsort (excl_blk_list,
                       pblk_list[EXCL_BLK_TYPE].count,
                       sizeof (u_int32_t),
                       lp_client_pkt_lbl_set_compare);
          }
        }
    }

  /* Also, init the message with inclusive and exclusive block list */
  for (ctr = 0; ctr < 2; ctr++)
    {
      if (! pblk_list[ctr].count)
        {
          continue;
        }

      msg_lset.blk_list[ctr].action_type  = pblk_list[ctr].action_type;
      msg_lset.blk_list[ctr].blk_req_type = BLK_REQ_TYPE_GET_LS_BLOCK;
      msg_lset.blk_list[ctr].blk_count    = pblk_list[ctr].count;

      if (ctr == INCL_BLK_TYPE)
        {
          msg_lset.blk_list[ctr].lset_blocks  = incl_blk_list;
        }
      else if (ctr == EXCL_BLK_TYPE)
        {
          msg_lset.blk_list[ctr].lset_blocks  = excl_blk_list;
        }
    }

  /* Request the server for a block */
  ret = lp_client_pkt_request_block_send (zg, table, lbl_space, nc,
                                          msg_lset.blk_list, service_req);
  if (ret < 0)
    {
      zlog_err (zg, "LPC: Could not get lset-label block for label "
                "space %d from NSM", lbl_space);

      XFREE (MTYPE_LABEL_SET_NODE, incl_blk_list);
      XFREE (MTYPE_LABEL_SET_NODE, excl_blk_list);
      return LP_CLIENT_LBL_SET_MSG_SEND_ERROR;
    }

  if (label_req_type == BLK_REQ_TYPE_EXPLICIT_LABEL)
    {
      /* Request for an explicit label; validate the requested label */
      ret = lp_client_pkt_get_label_from_blk (zg, client,
                                      msg_lset.blk_list,
                                      INCL_BLK_TYPE, pblk_list,
                                      glabel_ret, service_req);
    }
  else if (CHECK_FLAG (msg_lset.blk_list[INCL_BLK_TYPE].action_type,
                                                 ACTION_TYPE_INCLUDE_RANGE) &&
      CHECK_FLAG (msg_lset.blk_list[INCL_BLK_TYPE].action_type,
                                                 ACTION_TYPE_INCLUDE_LIST))
    {
      /* Include range/list based block returned; check for any exclusion */
      ret = lp_client_pkt_get_label_from_lset_blks (zg, client,
                                      msg_lset.blk_list,
                                      INCL_BLK_TYPE, pblk_list, plsol,
                                      glabel_ret, service_req);
    }
  else if (CHECK_FLAG (msg_lset.blk_list[INCL_BLK_TYPE].action_type,
                                                 ACTION_TYPE_EXCLUDE_RANGE) &&
           CHECK_FLAG (msg_lset.blk_list[INCL_BLK_TYPE].action_type,
                                                 ACTION_TYPE_EXCLUDE_LIST))
    {
      /* Exclude range/list based block returned; return the first available
       * label based on stored data for this block */
        ret = lp_client_pkt_get_label_from_lset_blks (zg, client,
                                          msg_lset.blk_list,
                                          EXCL_BLK_TYPE, pblk_list, plsol,
                                          glabel_ret, service_req);
    }

  /* Free up the block lists used in the request message and reply message */
  XFREE (MTYPE_LABEL_SET_NODE, incl_blk_list);
  XFREE (MTYPE_LABEL_SET_NODE, excl_blk_list);
  return ret;
}

/* Get the first usable label based on the data stored in pblk_list */
/* If exclusion check passes; return the label
 * else run through all the labels applicable for this block, based on count
 */
int
lp_client_pkt_get_label_from_lset_blks (struct lib_globals *zg,
                             struct lp_client *client,
                             struct nsm_msg_pkt_block_list *nlset_bl,
                             u_int8_t blk_type,
                             struct lp_client_pkt_lso_blk_list *pblk_list,
                             struct lp_client_pkt_lso_list *plsol,
                             struct generalized_label *glabel_ret,
                             enum label_pool_id serv_module)
{
  u_int32_t bc = 0;
  u_int32_t label = 0;
  struct lp_client_pkt_lso_blk *node = NULL;
  struct lp_client_pkt_lso_blk *nfound = NULL;
  struct bitmap_block *rblock = NULL;
  int serv_index;
  bool_t found = PAL_FALSE;

  serv_index = lp_client_get_service_index (serv_module);
  if (serv_index == LABEL_POOL_SERVICE_ERROR)
    {
       zlog_err (zg, "LPC: Invalid service type service %d", serv_module);
       return LP_CLIENT_FAILURE;
    }

  /* Add all the blocks returned by the server. Count shall be 1 for now. */
  for (bc = 0; bc < nlset_bl[blk_type].blk_count; bc++)
    {
      /* Add a new node to the list. */
      bitmap_add_new_node (client->block_list[serv_index],
                           nlset_bl[blk_type].lset_blocks[bc],
                           BITMAP_NODE_ADD_ON_DEMAND);
    }

  /* Look up the first block in the pblk_list */
  for (node = pblk_list[blk_type].head;   node; node = node->next)
    {
      if (node->pkt_block == nlset_bl[blk_type].lset_blocks[0])
        {
          /* Found the block returned by server in the client block list */
          nfound = node;
          break;
        }
    }
  if (! nfound)
    {
      /* Code flow broken */
      zlog_err (zg, "LPC: Could not find block %d for block type %d ",
                nlset_bl[blk_type].lset_blocks[0], blk_type);
      glabel_ret->u.mpls_label = LABEL_VALUE_INVALID;
      return LP_CLIENT_PKT_LBL_MATCH_FOUND;
    }

  /* Check exclusion range, list for IR or IL for the valid label range */
  if (blk_type == INCL_BLK_TYPE)
    {
      for (label = nfound->start_label; label <= nfound->lbl_count; label++)
        {
          if (lp_client_pkt_check_for_exclusion(&label, plsol,
                 ACTION_TYPE_EXCLUDE_LIST | ACTION_TYPE_EXCLUDE_RANGE)
                                                               == PAL_TRUE)
            {
              found = PAL_TRUE;
              break;
            }
        }

      if (! found)
        {
          return LP_CLIENT_PKT_LBL_MATCH_NOT_FOUND;
        }
    }

  if (blk_type == EXCL_BLK_TYPE)
    {
      /* Return the first valid label as noted in the client block list */
      label = nfound->start_label;
      found = PAL_TRUE;
    }

  rblock = bitmap_block_get_by_id (client->block_list[serv_index],
                                   nfound->pkt_block);
  if (found)
    {
      if (bitmap_request_index_by_block (client->block_list[serv_index],
                                             rblock, label) == BITMAP_SUCCESS)
        {
#ifdef HAVE_LPC_DEBUG
          zlog_info (zg, "LPC: Allocated lbl %d for block %d ",
                         label, rblock->id);
#endif
          glabel_ret->u.mpls_label = label;
          return LP_CLIENT_PKT_LBL_MATCH_FOUND;
        }
    }

  /* If we are here means something went wrong  or
   * None of the labels requested by the client are available in the
   * label space. Indicate with an error. */
  zlog_err (zg, "LPC: Could not find a label with server label-set-block");
  glabel_ret->gmpls_label_type = GMPLS_LABEL_ERROR;
  glabel_ret->u.mpls_label = LABEL_VALUE_INVALID;

  return LP_CLIENT_PKT_LBL_MATCH_NOT_FOUND;
}

/* Get a label from a block returned by the lp server based on an explicit
 * request for a particular block */
int
lp_client_pkt_get_label_from_blk (struct lib_globals *zg,
                             struct lp_client *client,
                             struct nsm_msg_pkt_block_list *nlset_bl,
                             u_int8_t blk_type,
                             struct lp_client_pkt_lso_blk_list *pblk_list,
                             struct generalized_label *glabel_ret,
                             enum label_pool_id serv_module)
{
  struct lp_client_pkt_lso_blk *node = NULL;
  struct lp_client_pkt_lso_blk *nfound = NULL;
  struct bitmap_block *rblock = NULL;
  int serv_index;

  serv_index = lp_client_get_service_index (serv_module);
  if (serv_index == LABEL_POOL_SERVICE_ERROR)
    {
       zlog_err (zg, "LPC: Invalid service type service %d", serv_module);
       return LP_CLIENT_FAILURE;
    }

  if (nlset_bl[blk_type].blk_count > 1)
    {
      zlog_err (zg, "LPC: Expected only 1 block %d",
                                              nlset_bl[blk_type].blk_count);
      return LP_CLIENT_FAILURE;
    }

  /* Add the single block returned by the server */
  bitmap_add_new_node (client->block_list[serv_index],
                       nlset_bl[blk_type].lset_blocks[0],
                       BITMAP_NODE_ADD_ON_DEMAND);

  /* Look up the first block in the pblk_list */
  node = pblk_list[blk_type].head;
  if (node->pkt_block == nlset_bl[blk_type].lset_blocks[0])
    {
      /* Found the block returned by server in the client block list */
      nfound = node;
    }

  if (! nfound)
    {
      /* Code flow broken */
      zlog_err (zg, "LPC: Could not find 1 block %d for block type %d ",
                nlset_bl[blk_type].lset_blocks[0], blk_type);
      glabel_ret->u.mpls_label = LABEL_VALUE_INVALID;
      return LP_CLIENT_PKT_LBL_MATCH_NOT_FOUND;
    }

  rblock = bitmap_block_get_by_id (client->block_list[serv_index],
                                   nfound->pkt_block);
  if (rblock)
    {
      if (bitmap_request_index_by_block (client->block_list[serv_index],
                                         rblock, glabel_ret->u.mpls_label)
                                                           == BITMAP_SUCCESS)
        {
#ifdef HAVE_LPC_DEBUG
          zlog_info (zg, "LPC: Alloccated single lbl %d for block %d ",
                         glabel_ret->u.mpls_label, rblock->id);
#endif
          return LP_CLIENT_PKT_LBL_MATCH_FOUND;
        }
    }

  /* If we are here means something went wrong  or
   * the labels requested by the client is not available in the
   * label space. Indicate with an error. */
  zlog_err (zg, "LPC: Could not find a label with server for single block");
  glabel_ret->gmpls_label_type = GMPLS_LABEL_ERROR;
  glabel_ret->u.mpls_label = LABEL_VALUE_INVALID;

  return LP_CLIENT_PKT_LBL_MATCH_NOT_FOUND;
}

/* Compare function for the label set lists in the host byte order; used for
 * both packet label and blocks*/
int
lp_client_pkt_lbl_set_compare (const void *a1, const void *a2)
{
  if (*(u_int32_t*) a1  < *(u_int32_t*) a2)
    {
      return -1;
    }

  if (*(u_int32_t*) a1  > *(u_int32_t*) a2)
    {
      return 1;
    }

  return 0;
}

int
lp_client_pkt_add_blk_to_list(struct lp_client_pkt_lso_blk_list *pblk,
                        u_int32_t *blk_id,
                        u_int32_t start_lbl,
                        u_int32_t count)
{
  struct   lp_client_pkt_lso_blk *node= NULL;

  node = XCALLOC(MTYPE_LABEL_SET_NODE, sizeof (struct lp_client_pkt_lso_blk));

  if (! node)
    {
      return LP_CLIENT_MEMORY_ALLOC_ERROR;
    }

  node->pkt_block = *blk_id;
  node->start_label = start_lbl;
  node->lbl_count = count;
  LP_CLIENT_PKT_LSO_ADD_TAIL (pblk, node);

  return LP_CLIENT_SUCCESS;
}

int
lp_client_pkt_process_include_list (struct lib_globals *zg,
                             struct lp_client *client,
                             struct lp_client_pkt_lso_list *plsol,
                             struct lp_client_pkt_lso_blk_list *pblk,
                             u_int32_t *ret_label,
                             enum label_pool_id serv_module)
{
  struct bitmap_block *rblock = NULL;
  u_int32_t count = 0;
  u_int32_t blk_id = 0;
  u_int32_t unused = 0;
  u_int32_t max_lbl_val = 0;
  u_int32_t curr_lbl = 0;
  struct   lp_client_pkt_lso *node = NULL;
  struct bitmap* blk_list = NULL;
  struct lp_client_pkt_lso_list *plso = &plsol[INCL_LIST_INDEX];
  int rc = LP_CLIENT_PKT_LBL_MATCH_NOT_FOUND;
  bool_t exclusion_succ = PAL_FALSE;
  int serv_index;

  serv_index = lp_client_get_service_index (serv_module);
  if (serv_index == LABEL_POOL_SERVICE_ERROR)
    {
       return LP_CLIENT_FAILURE;
    }

  blk_list = client->block_list[serv_index];

  for (node = plso->head; node; node = node->next)
    {
      while (count < node->pkt_label_count)
        {
          curr_lbl = node->pkt_labels[count];
          blk_id =  curr_lbl / blk_list->block_size;
          rblock = bitmap_block_get_by_id (blk_list, blk_id);

          if (! rblock)
            {
              /* Block not available; potential candiate for server to check */
              rc = lp_client_pkt_add_blk_to_list (pblk, &blk_id, curr_lbl,
                                 LP_CLIENT_PKT_ONE_ELEM);
              if (rc != LP_CLIENT_SUCCESS)
                {
                  return (rc);
                }
              /* Check other elements in the list */
              continue;
            }

           unused = bitmap_is_block_usable (blk_list, rblock);
           max_lbl_val = ((blk_id + 1) * blk_list->block_size) - 1;

           /* Found a block with matching index; check if the current element
            * in the list or any other elements match with one of the unused
            * indexes
            */
           exclusion_succ = lp_client_pkt_check_for_exclusion(ret_label, plsol,
                         ACTION_TYPE_EXCLUDE_LIST | ACTION_TYPE_EXCLUDE_RANGE);
           while (unused && exclusion_succ)
             {
                if (bitmap_request_index_by_block (blk_list, rblock,
                                          curr_lbl) == BITMAP_SUCCESS)
                  {
                    *ret_label = curr_lbl;
                    return LP_CLIENT_PKT_LBL_MATCH_FOUND;
                  }

               /* Scan as many labels as possible for the current block */
               count++;
               unused--;
               curr_lbl = node->pkt_labels[count];
             }

           /* No match for the curr label; move to the next one in the list */
           count++;
        }
    }
  return rc;
}

/* For exclude lists, first the incoming list is sorted thereby
 * determining the min, max of the list, then 1, 2, 3 are applied:
 * 1) If the max is not the upper limit of the label space, then the
 *    label is allocated within max of list and upper limit of LS
 * 2) Otherwise, attempt to get a label within lower limit of LS and
 *    min of this exclude list, if such a range exists
 * 3) Otherwise, attempt to allocate a label from among the possible
 *    sub-ranges that can be derieved between two consecutine elements
 *    in the sorted list, like E1-E2 or E2-E3,..., E(n-1)-E(n) which may
 *    have a sub-range among them
 */
int
lp_client_pkt_process_exclude_list (struct lib_globals *zg,
                             struct lp_client *client,
                             struct lp_client_pkt_lso_list *plsol,
                             struct lp_client_pkt_lso_blk_list *pblk,
                             u_int32_t *ret_label,
                             enum label_pool_id serv_module)
{
  u_int32_t count = 0;
  u_int32_t ls_min = 0;
  u_int32_t ls_max = 0;
  u_int32_t el_min = 0;
  u_int32_t el_max = 0;
  u_int32_t el_length = 0;
  struct   lp_client_pkt_lso *node = NULL;
  struct lp_client_pkt_lso_list *plso = &plsol[EXCL_LIST_INDEX];
  int rc = LP_CLIENT_PKT_LBL_MATCH_NOT_FOUND;
  int serv_index;

  serv_index = lp_client_get_service_index (serv_module);
  if (serv_index == LABEL_POOL_SERVICE_ERROR)
    {
       return LP_CLIENT_FAILURE;
    }

  /* Get relevant data */
  ls_min = client->block_list[serv_index]->min_index;
  ls_max = client->block_list[serv_index]->max_index;

  for (node = plso->head; node; node = node->next)
    {
      el_length = node->pkt_label_count;
      el_min = node->pkt_labels[0];
      el_max = node->pkt_labels[el_length - 1];

      /* Check for room at the beginning of the label space to find a label */
      if (ls_min < el_min)
        {
          rc = lp_client_pkt_find_label_in_range(zg, client, ls_min, (el_min - 1),
                                   plsol, pblk, ACTION_TYPE_EXCLUDE_LIST,
                                   ret_label, serv_module);
        }
      /* Check for room at the end of the label space to find a label */
      else if ((rc != LP_CLIENT_PKT_LBL_MATCH_FOUND) &&
               (ls_max > el_max))
        {
          rc = lp_client_pkt_find_label_in_range(zg, client, (el_max + 1),
                             ls_max, plsol, pblk, ACTION_TYPE_EXCLUDE_LIST,
                             ret_label, serv_module);
        }
      /* Need to find a label within the exclude list elements */
      else if (rc != LP_CLIENT_PKT_LBL_MATCH_FOUND)
        {
          /* Find any sub-range between two consecutive elements and try to
           * allocate a label from such a range */
          for (count = 0;
               (count < el_length && rc != LP_CLIENT_PKT_LBL_MATCH_FOUND);
               count++)
            {
              if (((node->pkt_labels[count + 1]) - node->pkt_labels[count]) > 1)
                {
                  /* Found a range between current and next element */
                  rc = lp_client_pkt_find_label_in_range(zg, client,
                                      node->pkt_labels[count] + 1,
                                      (node->pkt_labels[count + 1] - 1),
                                      plsol, pblk, ACTION_TYPE_EXCLUDE_LIST,
                                      ret_label, serv_module);
                }
            }
        }
    }
  return rc;
}

int
lp_client_pkt_lset_get_range_intersection (struct lp_client *client,
                               struct lp_client_glabel_list *glbl_list,
                               u_int32_t *min_val,
                               u_int32_t *max_val,
                               enum label_pool_id serv_module)
{
  int serv_index;

  serv_index = lp_client_get_service_index (serv_module);
  if (serv_index == LABEL_POOL_SERVICE_ERROR)
    {
       return LP_CLIENT_FAILURE;
    }

  /* Only two elelemts are expected in the label set for ranges */
  if (glbl_list->glabel_count != LS_MAX_RANGE_ELEMENTS)
    {
      return LP_CLIENT_PKT_INVALID_LSET_DATA;
    }

  *min_val = glbl_list->glabels[0].u.mpls_label;
  *max_val = glbl_list->glabels[1].u.mpls_label;

  /* Determine if we have a unbounded upper limit */
  if (*max_val == LS_UNBOUND_UPPER_LIMIT)
    {
      *max_val = client->block_list[serv_index]->max_index;
    }

  /* Determine the intersecting label range */
  if (*min_val < client->block_list[serv_index]->min_index)
    {
      *min_val = client->block_list[serv_index]->min_index;
    }
  if (*max_val > client->block_list[serv_index]->max_index)
    {
      *max_val = client->block_list[serv_index]->max_index;
    }

  return LP_CLIENT_SUCCESS;
}

bool_t
lp_client_pkt_check_for_exclusion(u_int32_t *label,
                            struct lp_client_pkt_lso_list *plsol,
                            u_int8_t action_type)
{
  /* Verify that the label is not part of any exclude ranges
   * Verify that the label is not part of any exclude list
   */
  u_int32_t iter = 0;
  u_int32_t exclctr = 0;
  u_int32_t lc = 0;
  struct lp_client_pkt_lso_list *plso_excl = NULL;
  struct lp_client_pkt_lso *node = NULL;

  if (CHECK_FLAG (action_type, ACTION_TYPE_EXCLUDE_RANGE) ||
      CHECK_FLAG (action_type, ACTION_TYPE_EXCLUDE_LIST))
    {
      /* Both ER and EL are assumed to be mutually exclusinve, Hence,
       * any label found outside of these two lists is always usable*/
      return PAL_TRUE;;
    }

  /* Being here means that validation is required for either IR or IL */

  for (exclctr = EXCL_RANGE_INDEX; exclctr <= EXCL_LIST_INDEX; exclctr -= 2)
    {
      iter = plsol[exclctr].count;

      while (iter)
        {
          plso_excl = &plsol[exclctr];
          for (node = plso_excl->head; node; node = node->next)
            {
              for (lc = 0; lc < node->pkt_label_count; lc++)
                {
                  if (*label == node->pkt_labels[lc])
                    {
                      return PAL_TRUE;
                    }
                }
            }
          iter--;
        }
    }

  return PAL_FALSE;
}

int
lp_client_pkt_find_label_in_range (struct lib_globals *zg,
                            struct lp_client *client,
                            u_int32_t min_range,
                            u_int32_t max_range,
                            struct lp_client_pkt_lso_list *plsol,
                            struct lp_client_pkt_lso_blk_list *pblk,
                            u_int8_t action_type,
                            u_int32_t *ret_label,
                            enum label_pool_id serv_module)
{
  int blk_id = LABEL_BLOCK_INVALID;
  u_int32_t label = 0;
  u_int32_t skip_count = 0;
  u_int32_t failed_lbl = LABEL_VALUE_INVALID;
  struct bitmap_block *rblock = NULL;
  struct bitmap *blk_list = NULL;
  int rc = LP_CLIENT_PKT_LBL_MATCH_NOT_FOUND;
  int blk_add = LP_CLIENT_FAILURE;
  bool_t unusabel_block = PAL_FALSE;
  int free_label = 0;
  int serv_index = 0;
  int first_free_label = LABEL_VALUE_INVALID;
  bool_t found = PAL_FALSE;

  serv_index = lp_client_get_service_index (serv_module);
  if (serv_index == LABEL_POOL_SERVICE_ERROR)
    {
       return LP_CLIENT_FAILURE;
    }

#ifdef HAVE_LPC_DEBUG
  zlog_info (zg, "LPC-S: find_label_in_range min %d max %d action_type %d",
                  min_range, max_range, action_type);
#endif

  blk_list = client->block_list[serv_index];

  /* Iterate through the range to find an un-used label in client scope */
  label = min_range;

  while (label <= max_range)
    {
      /*  Find the block corresponding to the index */
      blk_id = label / blk_list->block_size;
      rblock = (bitmap_block_get_by_id (blk_list, blk_id));

      if (rblock)
        {
          rc = bitmap_is_block_usable (blk_list, rblock);

          first_free_label = bitmap_get_first_free_index_by_block (blk_list,
                                                                      rblock);
          if (first_free_label > max_range)
            {
              /* All the labels in the label set are in use; bail out */
              zlog_warn (zg, "LPC: All labels in the lset are in use");
              return LP_CLIENT_LBL_SET_LBL_NOT_AVAIL;
            }

          if (rc)
            {
              while ((rc > 0) && (found == PAL_FALSE))
                {
                  /* run through all the free labels, start with the first */
                  free_label = bitmap_get_first_free_index_by_block (blk_list,
                                                                   rblock);
                  /* Check for any restrictions on the available one*/
                  if (lp_client_pkt_check_for_exclusion(&free_label, plsol,
                        ACTION_TYPE_EXCLUDE_LIST | ACTION_TYPE_EXCLUDE_RANGE)
                                                                  == PAL_TRUE)
                    {
                      /* See if the label can be used */
                      if (bitmap_request_index_by_block (blk_list, rblock,
                                                free_label) == BITMAP_SUCCESS)
                        {
                          *ret_label = free_label;
                          found = PAL_TRUE;
#ifdef HAVE_LPC_DEBUG
                          zlog_info (zg, "LPC: Lset allocated label %d",
                                                                  free_label);
#endif
                          return LP_CLIENT_PKT_LBL_MATCH_FOUND;
                        }
                    }

                  rc--;
                }
              /* Skip the current block */
              label = blk_id * blk_list->block_size;
            }
          else
            {
              unusabel_block = PAL_TRUE;
            }
        }

      if ((! rblock) || (unusabel_block == PAL_TRUE))
        {
          /* Unusable block, find the start index of the next block  */
          skip_count = ((blk_id + 1) * blk_list->block_size) - 1;

          /* Step ahead to the next block */
          failed_lbl = label;
          label += (skip_count - label) + 1;

          if (skip_count > max_range)
            {
              skip_count = max_range;
            }
          unusabel_block = PAL_FALSE;
        }

     if (!rblock)
      {
        /* Block not available; potential candiate for server to check */
        blk_add = lp_client_pkt_add_blk_to_list (pblk, &blk_id, failed_lbl,
                                                 skip_count);
        if (blk_add != LP_CLIENT_SUCCESS)
          {
            return (blk_add);
          }
      }
    }

  zlog_warn (zg, "LPC-S: find_label_in_range - could not locate a label");

  return LP_CLIENT_PKT_LBL_MATCH_NOT_FOUND;
}

int
lp_client_pkt_process_include_range (struct lib_globals *zg,
                               struct lp_client *client,
                               struct lp_client_pkt_lso_list *plsol,
                               struct lp_client_pkt_lso_blk_list *pblk,
                               u_int32_t *ret_label,
                               enum label_pool_id serv_module)
{
  struct lp_client_pkt_lso *node = NULL;
  struct lp_client_pkt_lso_list *plso = &plsol[INCL_RANGE_INDEX];
  int rc = LP_CLIENT_PKT_LBL_MATCH_NOT_FOUND;

  for (node = plso->head; node; node = node->next)
    {
      rc = lp_client_pkt_find_label_in_range(zg, client, node->pkt_labels[0],
                                         node->pkt_labels[1], plsol, pblk,
                                         ACTION_TYPE_INCLUDE_RANGE,
                                         ret_label, serv_module);

      if (rc == LP_CLIENT_PKT_LBL_MATCH_FOUND)
        {
          break;
        }
    }

  return rc;
}

/* Within the range, during each iteration, try label allocation
 * from both the ends of the range converging towards the middle.
 * The intent is to avoid contineous failures by going from one
 * end to the other end
 */
int
lp_client_pkt_process_exclude_range (struct lib_globals *zg,
                               struct lp_client *client,
                               struct lp_client_pkt_lso_list *plsol,
                               struct lp_client_pkt_lso_blk_list *pblk,
                               u_int32_t *ret_label,
                               enum label_pool_id serv_module)
{
  u_int32_t ls_min;
  u_int32_t ls_max;
  u_int32_t er_min;
  u_int32_t er_max;
  struct lp_client_pkt_lso *node = NULL;
  struct lp_client_pkt_lso_list *plso = &plsol[EXCL_RANGE_INDEX];
  int rc = LP_CLIENT_PKT_LBL_MATCH_NOT_FOUND;
  int serv_index;

  serv_index = lp_client_get_service_index (serv_module);
  if (serv_index == LABEL_POOL_SERVICE_ERROR)
    {
       return LP_CLIENT_FAILURE;
    }

  /* Get relevant data */
  ls_min = client->block_list[serv_index]->min_index;
  ls_max = client->block_list[serv_index]->max_index;

  for (node = plso->head; node; node = node->next)
    {
      er_min = node->pkt_labels[0];
      er_max = node->pkt_labels[1];

      /* Check for room at the beginning of the label space to find a label */
      if (ls_min < er_min)
        {
          rc = lp_client_pkt_find_label_in_range(zg, client, ls_min,
                                        (er_min - 1), plsol, pblk,
                                        ACTION_TYPE_EXCLUDE_RANGE,
                                        ret_label, serv_module);
        }
      /* Check for room at the end of the label space to find a label */
      else if ((rc != LP_CLIENT_PKT_LBL_MATCH_FOUND) &&
               (ls_max > er_max))
        {
          rc = lp_client_pkt_find_label_in_range(zg, client, (er_max + 1),
                                         ls_max, plsol, pblk,
                                         ACTION_TYPE_EXCLUDE_RANGE,
                                         ret_label, serv_module);
        }
  }

  return rc;
}

/* This API provides two services:
 * (1) Request the allocation of a suggested label
 * (2) Request the allocation of an explicit label
 */
int
lp_client_pkt_get_label_by_value (struct lib_globals *zg,
                        struct route_table *table,
                        u_int16_t lbl_space,
                        struct nsm_client *nc,
                        u_int8_t type_of_service,
                        struct generalized_label  *gen_label, /* arg, return */
                        struct lp_client_label_set *lbl_set,  /* arg */
                        enum label_pool_id serv_module)
{
  u_int32_t ret_lbl = LABEL_VALUE_INVALID;
  struct lp_client *client = NULL;
  struct generalized_label loc_glbl;
  int ret = LP_CLIENT_SUGG_LBL_ERROR;
  int retls = LP_CLIENT_LBL_SET_ERROR;

  pal_mem_set (&loc_glbl, 0, sizeof (struct generalized_label));

  client = lp_client_lookup (zg, table, lbl_space);
  if (! client)
    {
      return LABEL_VALUE_INVALID;
    }

  if (! gen_label)
    {
      return LP_CLIENT_PKT_LBL_MATCH_NOT_FOUND;
    }

  if (gen_label->gmpls_label_type != GMPLS_LABEL_PACKET)
    {
      return LP_CLIENT_INVALID_LABEL_TYPE;
    }

  switch (type_of_service)
    {
      case BLK_REQ_TYPE_SUGGESTED_LABEL:
        {
          ret = lp_client_pkt_request_label_by_value (zg, table, lbl_space,
                                                      nc, client,
                                                      LP_CLIENT_PKT_FLAG_NONE,
                                                      gen_label,
                                                      serv_module);

          if (ret == LP_CLIENT_PKT_LBL_MATCH_FOUND)
            {
#ifdef HAVE_LPC_DEBUG
              zlog_info (zg, "LPC: sugg-lbl get_by_val success for lbl %d",
                             gen_label->u.mpls_label);
#endif
              return ret;
            }

          /* Suggesetd label not available; try the label set if it exists */
          if (lbl_set->ls_obj_count > 0)
            {
              retls = lp_client_pkt_get_label_from_label_set (zg, table,
                                                    lbl_space, nc, lbl_set,
                                                    gen_label, serv_module);
              if (retls == LP_CLIENT_PKT_LBL_MATCH_FOUND)
                {
#ifdef HAVE_LPC_DEBUG
                  zlog_info (zg, "LPC: sugg Lbl from lset - success for lbl %d",
                                 gen_label->u.mpls_label);
#endif
                }
              else
                {
                  gen_label->gmpls_label_type = GMPLS_LABEL_ERROR;
                  gen_label->u.mpls_label = LABEL_VALUE_INVALID;
                  zlog_err (zg, "LPC: sugg-lbl from lset - failed ");
                }

              /* For either success or failure, a request for suggested label
               * shall return at this point with the presence of a label set */
              return retls;
            }

          /* Suggested label is not available, and label set is not preset;
           * Now find the next available label at the client scope */
          if ((ret == LP_CLIENT_PKT_LBL_MATCH_NOT_FOUND) ||
              (lbl_set->ls_obj_count == 0))
            {
              ret_lbl = lp_client_pkt_request_label (zg, table, lbl_space,
                                                     nc, serv_module);

              if (! VALID_LABEL (ret_lbl))
                {
                  zlog_err (zg, "LPC: Sugg lbl request to lpclient failed");
                  gen_label->gmpls_label_type = GMPLS_LABEL_ERROR;
                  gen_label->u.mpls_label = LABEL_VALUE_INVALID;
                  return LP_CLIENT_PKT_LBL_MATCH_NOT_FOUND;
                }

              /* Success; return another label instead of suggested label */
              gen_label->u.mpls_label = ret_lbl;
#ifdef HAVE_LPC_DEBUG
              zlog_info (zg, "LPC: sugg Lbl from lpc - success for lbl %d",
                             gen_label->u.mpls_label);
#endif
            }
          break;
        }

      case BLK_REQ_TYPE_EXPLICIT_LABEL:
        {
          ret_lbl = gen_label->u.mpls_label;

          ret = lp_client_pkt_request_label_by_value (zg, table, lbl_space, nc,
                                          client,
                                          LP_CLIENT_PKT_REQ_LABEL_FROM_SERVER,
                                          gen_label, serv_module);

          /* Can't allocate the requested explict label */
          if (ret == LP_CLIENT_PKT_LBL_MATCH_NOT_FOUND)
            {
              zlog_err (zg, "LPC: Exp lbl request to lpclient failed for %d",
                             ret_lbl);
              gen_label->gmpls_label_type = GMPLS_LABEL_ERROR;
              gen_label->u.mpls_label = LABEL_VALUE_INVALID;
              return LP_CLIENT_EXP_LBL_MATCH_NOT_FOUND;
            }
          break;
        }

      default:
        return LP_CLIENT_INVALID_SERVICE_TYPE;
    }

    return LP_CLIENT_PKT_LBL_MATCH_FOUND;
}

/* Allocate a given label */
int
lp_client_pkt_request_label_by_value (struct lib_globals *zg,
                               struct route_table *table,
                               u_int16_t lbl_space,
                               struct nsm_client *nc,
                               struct lp_client *client,
                               u_int8_t flags,
                               struct generalized_label *gen_label,
                               enum label_pool_id serv_module)
{
  u_int32_t blk_id = LABEL_BLOCK_INVALID;
  u_int32_t max_lbl_val = LABEL_VALUE_INVALID;
  u_int32_t unused = LABEL_VALUE_INVALID;
  int rc = LP_CLIENT_PKT_LBL_MATCH_NOT_FOUND;
  struct bitmap_block *rblock = NULL;
  struct bitmap *blk_list = NULL;
  int blk_add = LP_CLIENT_FAILURE;
  u_int32_t lbl_req = 0;
  struct lp_client_pkt_lso_blk_list pblk_list[LSO_BLK_TYPE_MAX];
  int serv_index;
  int type;

  if (gen_label)
    {
      lbl_req = gen_label->u.mpls_label;
    }
  serv_index = lp_client_get_service_index (serv_module);
  if (serv_index == LABEL_POOL_SERVICE_ERROR)
    {
       return LP_CLIENT_FAILURE;
    }
  blk_list = client->block_list[serv_index];

  blk_id =  lbl_req / blk_list->block_size;
  rblock = bitmap_block_get_by_id (blk_list, blk_id);
  max_lbl_val = ((blk_id + 1) * blk_list->block_size) - 1;

  if (rblock)
    {
      if (lbl_req > max_lbl_val)
        {
          return rc;
        }

      unused = bitmap_is_block_usable (blk_list, rblock);

      if (unused)
        {
          if (bitmap_request_index_by_block (blk_list, rblock,
                                                     lbl_req) == BITMAP_SUCCESS)
            {
              rc = LP_CLIENT_PKT_LBL_MATCH_FOUND;
            }
        }
    }

  /* Request the explicit label from LP server */
  if ((rc != LP_CLIENT_PKT_LBL_MATCH_FOUND) &&
      (CHECK_FLAG (flags, LP_CLIENT_PKT_REQ_LABEL_FROM_SERVER)))
    {
      for (type = LSO_BLK_TYPE_MIN; type < LSO_BLK_TYPE_MAX; type++)
        {
          pblk_list[type].action_type = 0;
          pblk_list[type].count = 0;
          pblk_list[type].head = NULL;
          pblk_list[type].tail = NULL;
        }

      blk_add = lp_client_pkt_add_blk_to_list (&pblk_list[INCL_BLK_TYPE],
                                               &blk_id, lbl_req, 0);
      if (blk_add != LP_CLIENT_SUCCESS)
        {
          return (blk_add);
        }

      rc = lp_client_pkt_req_on_demand_block_send (zg, table, lbl_space,
                                     nc, NULL, pblk_list, gen_label,
                                     BLK_REQ_TYPE_EXPLICIT_LABEL, serv_module);

    }

  return rc;
}

#if defined HAVE_RSVP_GRST || defined HAVE_RESTART

int
lp_client_pkt_reserve_label (struct lib_globals *zg,
                         struct route_table *table,
                         u_int16_t label_space,
                         u_int32_t label,
                         enum label_pool_id serv_module)
{
  struct lp_client *client;
  int ret;
  int serv_index;

  serv_index = lp_client_get_service_index (serv_module);
  if (serv_index == LABEL_POOL_SERVICE_ERROR)
    {
       zlog_err (zg, "LPC: Invalid service type service %d", serv_module);
       return LP_CLIENT_FAILURE;
    }

  client = lp_client_lookup (zg, table, label_space);
  if (! client)
    return LP_CLIENT_FAILURE;

  ret = bitmap_reserve_index (client->block_list[serv_index], label);
  if (ret == BITMAP_FAILURE)
    return LP_CLIENT_FAILURE;
  else
    return LP_CLIENT_SUCCESS;
}

/* if mark_stale is true then the block is set as stale; ottherwise the block
 * is marked unstale */
int
lp_client_pkt_mark_block_stale(struct lib_globals *zg,
                          struct route_table *table,
                          u_int16_t lbl_space,
                          bool_t mark_stale,
                          enum label_pool_id serv_module)
{
  u_int32_t j;
  struct lp_client *client;
  struct bitmap_block *block = NULL;
  struct bitmap_block *block_next = NULL;
  int serv_index;

  serv_index = lp_client_get_service_index (serv_module);
  if (serv_index == LABEL_POOL_SERVICE_ERROR)
    {
       zlog_err (zg, "LPC: Invalid service type service %d", serv_module);
       return LP_CLIENT_FAILURE;
    }

  client = lp_client_lookup (zg, table, lbl_space);
  if (! client)
    {
      return LP_CLIENT_FAILURE;
    }

  if (client->block_list[serv_index])
    {
      zlog_err (zg, "LPC: Failed to find the label space client");
      return LP_CLIENT_BLOCK_LIST_GET_ERROR;
    }

  for (j = 0; j < BITMAP_HASH_SIZE; j++)
    {
      for (block = client->block_list[serv_index]->hash[j]; block;
           block = block_next)
        {
          block_next = block->next;

          if (mark_stale)
            {
              SET_FLAG (block->flags, BITMAP_BLOCK_FLAG_STALE);
            }
          else
            {
              if (CHECK_FLAG (block->flags, BITMAP_BLOCK_FLAG_STALE))
                {
                  UNSET_FLAG (block->flags, BITMAP_BLOCK_FLAG_STALE);
                }
            }
        }
    }

  /* Make sure that this block does not get used for new labels */
  client->block_list[serv_index]->curr_node = NULL;

  return LP_CLIENT_SUCCESS;
}

#endif /* HAVE_RSVP_GRST  || HAVE_RESTART */

