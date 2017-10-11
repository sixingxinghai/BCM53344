/* Copyright (C) 2002-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#ifdef HAVE_TE

#include "table.h"
#include "message.h"
#ifdef HAVE_DSTE
#include "dste.h"
#endif /* HAVE_DSTE */
#include "nsm_message.h"
#include "nsm_client.h"
#include "qos_client.h"
#include "network.h"
#include "log.h"
#include "thread.h"
#include "label_pool.h"

int
qos_client_init_protocol (struct nsm_client_handler *nch, 
                          u_int32_t protocol_id, 
                          struct route_table **table)
{
  u_char *pnt;
  u_int16_t size;
  int nbytes;
  u_int32_t msg_id;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* Initialize the route table. */
  *table = route_table_init ();

  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size. */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode qos init. */
  nbytes = nsm_encode_qos_client_init (&pnt, &size, &protocol_id);
  if (nbytes < 0)
    return nbytes;
  nbytes =  nsm_client_send_message (nch, vr_id, vrf_id,
                                     NSM_MSG_QOS_CLIENT_INIT, nbytes, &msg_id);

  return nbytes;
}

#ifdef HAVE_GMPLS
int
gmpls_qos_client_init_protocol (struct nsm_client_handler *nch, 
                                u_int32_t protocol_id, 
                                struct route_table **table)
{
  u_char *pnt;
  u_int16_t size;
  int nbytes;
  u_int32_t msg_id;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* Initialize the route table. */
  *table = route_table_init ();

  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size. */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode qos init. */
  nbytes = nsm_encode_qos_client_init (&pnt, &size, &protocol_id);
  if (nbytes < 0)
    return nbytes;
  nbytes =  nsm_client_send_message (nch, vr_id, vrf_id,
                                     NSM_MSG_GMPLS_QOS_CLIENT_INIT, nbytes, &msg_id);
  return nbytes;
}
#endif /*HAVE_GMPLS*/

int
qos_client_release_slow (struct nsm_client *nc,
                         struct lib_globals *zg, 
                         struct route_table *table,
                         struct qos_resource *resource,
                         u_char free_resrc)
{
  int nbytes;
  struct nsm_msg_qos msg;
#ifdef HAVE_GMPLS
  struct nsm_msg_gmpls_qos gmpls_msg;
#endif /* HAVE_GMPLS */
  struct nsm_client_handler *nch;
  struct route_node *rn;
  struct prefix p;
  u_char *pnt;
  u_int16_t size;
  u_int32_t msg_id;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* Get sync response from async socket. Queue other messages in
     pending list.. */
  nch = nc->async;
  
  if (! nch || ! nch->up)
    return QOS_ERR;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  msg.resource_id = resource->resource_id;
  msg.protocol_id = resource->protocol_id;

#ifdef HAVE_GMPLS
  if (CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_GMPLS_IF_SPEC))
    {
      qos_prot_nsm_message_map_gmpls (resource, &gmpls_msg);

      /* Encode QOS message */
      nbytes = nsm_encode_gmpls_qos (&pnt, &size, &gmpls_msg);
    }
  else
#endif /* HAVE_GMPLS */
    {
      qos_prot_nsm_message_map (resource, &msg);

      /* Encode QOS message */
      nbytes = nsm_encode_qos (&pnt, &size, &msg);
    }

  if (nbytes < 0)
    return QOS_ERR;

  /* Send message. */
#ifdef HAVE_GMPLS
  if (CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_GMPLS_IF_SPEC))
    {
      nbytes =  nsm_client_send_message (nch, vr_id, vrf_id,
                                         NSM_MSG_GMPLS_QOS_CLIENT_RELEASE_SLOW,
                                         nbytes, &msg_id);
    }
  else
#endif /*HAVE_GMPLS*/ 
    {
      nbytes =  nsm_client_send_message (nch, vr_id, vrf_id,
                                         NSM_MSG_QOS_CLIENT_RELEASE_SLOW,
                                         nbytes, &msg_id);
    }

  /* Create 32 bit prefix */
  pal_mem_set (&p, 0, sizeof (struct prefix));
  p.family = AF_INET;
  p.prefixlen = IPV4_MAX_PREFIXLEN;
  PREP_FOR_NETWORK (p.u.prefix4.s_addr,
                    resource->resource_id,
                    IPV4_MAX_PREFIXLEN);

  /* Get route node */
  rn = route_node_lookup (table, &p);
  if (rn && rn->info)
    {
      /* Delete this resource and route node if free_resrc flag is set. */

      if ((resource = rn->info) != NULL)
        {
          /* If resource needs to be freed, delete it, else unset
             the resource id. */
          if (free_resrc == 1)
            qos_common_resource_delete (resource);
          else
            resource->resource_id = 0;
          
          /* Remove route node. */
          rn->info = NULL;
          route_unlock_node (rn);
          route_unlock_node (rn);
        }
    }
  
  return QOS_OK;
}

qos_status
qos_client_message (struct nsm_client *nc,
                    struct lib_globals *zg, 
                    struct route_table *table, int cmd,
                    struct qos_resource **resource)
{
  int ret_type;
  int nbytes;
  struct nsm_msg_qos msg;
  struct nsm_client_handler *nch;
  u_char *pnt;
  u_int16_t size;
  struct nsm_msg_header header;
  u_int32_t msg_id;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* Confirm command. */
  if (cmd != NSM_MSG_QOS_CLIENT_PROBE
      && cmd != NSM_MSG_QOS_CLIENT_RESERVE
      && cmd != NSM_MSG_QOS_CLIENT_MODIFY)
    return QOS_ERR;

  /* Get sync response from async socket. Queue other messages in
     pending list.. */
  nch = nc->async;
  
  if (! nch || ! nch->up || ! table)
    return QOS_ERR;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  msg.resource_id = (*resource)->resource_id;
  msg.protocol_id = (*resource)->protocol_id;

  qos_prot_nsm_message_map (*resource, &msg);

  /* Encode QOS message */
  nbytes = nsm_encode_qos (&pnt, &size, &msg);
  if (nbytes < 0)
    return QOS_ERR;

  /* Send message. */
  nbytes =  nsm_client_send_message (nch, vr_id, vrf_id, cmd, nbytes, &msg_id);
  if (nbytes < 0)
    return QOS_ERR;

  do
    {
      /* Sync wait for reply. */
      nbytes = nsm_client_read_sync (nch->mc,
                                     NULL,
                                     nch->mc->sock,
                                     &header,
                                     &ret_type);
      if (nbytes < 0)
        return QOS_ERR;

      if (ret_type != cmd) 
        {
          struct nsm_client_pend_msg *pmsg;

          /* Queue the message for later processing. */
          pmsg = XMALLOC (MTYPE_TMP, sizeof (struct nsm_client_pend_msg));

          pmsg->header = header;

          pal_mem_cpy (pmsg->buf, nch->pnt, nch->size);

          /* Add to pending list. */
          listnode_add (&nch->pend_msg_list, pmsg);

        }
    }
  while (ret_type != cmd);

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_qos));

  /* Set pnt and size. */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->size;

  /* Decode QOS response */
  nbytes = nsm_decode_qos (&pnt, &size, &msg);
  if (nbytes < 0)
    return QOS_ERR;

  /* Launch event to process pending requests. */
  if (!zg->pend_read_thread)
    zg->pend_read_thread =
      thread_add_read_pend (zg, nsm_client_process_pending_msg, nch, 0);
  
  if (nc->debug)
    nsm_qos_dump (zg, &msg, cmd);

  if (NSM_CHECK_CTYPE (msg.cindex, NSM_QOS_CTYPE_STATUS))
    {
      if (msg.status == QOS_OK)
        {
          struct route_node *rn;
          struct prefix p;

          /* Re initialize the resource */
          qos_common_resource_init (*resource);

          /* Re initialize the resource */
          qos_nsm_prot_message_map (&msg, *resource);

          if (cmd == NSM_MSG_QOS_CLIENT_RESERVE)
            {
              /*
               * Add resource to table based on resource id.
               */
          
              /* Create 32 bit prefix */
              pal_mem_set (&p, 0, sizeof (struct prefix));
              p.family = AF_INET;
              p.prefixlen = IPV4_MAX_PREFIXLEN;
              PREP_FOR_NETWORK (p.u.prefix4.s_addr,(*resource)->resource_id,
                                IPV4_MAX_PREFIXLEN);
          
              /* Get route node from table */
              rn = route_node_get (table, &p);
              pal_assert ((rn) && (! rn->info));

              /* Add resource */
              rn->info = *resource;
            }
          return QOS_OK;
        }
      else
        return msg.status;
    }
  else
    return QOS_ERR;
}

/* Non-blocking message for QoS. */
qos_status
qos_client_message_nb (struct nsm_client *nc,
                       struct lib_globals *zg, 
                       struct route_table *table,
                       int cmd,
                       struct qos_resource *resource,
                       u_int32_t *qos_msg_id)
{
  int nbytes;
  struct nsm_msg_qos msg;
#ifdef HAVE_GMPLS
  struct nsm_msg_gmpls_qos gmpls_msg;
#endif
  struct nsm_client_handler *nch;
  u_char *pnt;
  u_int16_t size;
  u_int32_t msg_id;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* Confirm command. */
  if (cmd != NSM_MSG_QOS_CLIENT_PROBE
      && cmd != NSM_MSG_QOS_CLIENT_RESERVE
      && cmd != NSM_MSG_QOS_CLIENT_MODIFY
#ifdef HAVE_GMPLS
      && cmd != NSM_MSG_GMPLS_QOS_CLIENT_PROBE
      && cmd != NSM_MSG_GMPLS_QOS_CLIENT_RESERVE
      && cmd != NSM_MSG_GMPLS_QOS_CLIENT_MODIFY
#endif /*HAVE_GMPLS*/
     )
    return QOS_ERR;

  /* Get sync response from async socket. Queue other messages in
     pending list.. */
  nch = nc->async;
  
  if (! nch || ! nch->up || ! table)
    return QOS_ERR;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  ++nch->mpls_msg_id;
  if (nch->mpls_msg_id == 0)
    nch->mpls_msg_id++;
  resource->id = nch->mpls_msg_id;

#ifdef HAVE_GMPLS
  if (CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_GMPLS_IF_SPEC))  
    {
      qos_prot_nsm_message_map_gmpls (resource, &gmpls_msg);
      /* Encode QOS message */
      nbytes = nsm_encode_gmpls_qos (&pnt, &size, &gmpls_msg);
    }
  else
#endif
    {
      qos_prot_nsm_message_map (resource, &msg);
      /* Encode QOS message */
      nbytes = nsm_encode_qos (&pnt, &size, &msg);
    }

  if (nbytes < 0)
    return QOS_ERR;

  /* Send message. */
  nbytes =  nsm_client_send_message (nch, vr_id, vrf_id, cmd, nbytes, &msg_id);
  if (nbytes < 0)
    return QOS_ERR;

  /* Set QoS message id.  */
  if (qos_msg_id)
    *qos_msg_id = nch->mpls_msg_id;

  return QOS_OK;
}

int
qos_client_release (struct nsm_client *nc,
                    struct lib_globals *zg, 
                    struct route_table *table,
                    u_int32_t resource_id,
                    u_int32_t protocol_id,
                    u_int32_t ifindex,
                    u_int8_t free_resrc)
{
  int nbytes;
  struct qos_resource *resource;
  struct route_node *rn;
  struct prefix p;
  struct nsm_client_handler *nch;
  u_char *pnt;
  u_int16_t size;
  struct nsm_msg_qos_release msg;
  u_int32_t msg_id;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* Use sync. */
  nch = nc->async;

  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  pal_mem_set (&msg, 0, sizeof(struct nsm_msg_qos_release));
  msg.resource_id = resource_id;
  msg.protocol_id = protocol_id;

  NSM_SET_CTYPE (msg.cindex, NSM_QOS_RELEASE_CTYPE_IFINDEX);
  msg.ifindex = ifindex;
  nbytes = nsm_encode_qos_release (&pnt, &size, &msg);

  if (nbytes < 0)
    return nbytes;

  /* Send message. */
  nbytes =  nsm_client_send_message (nch, vr_id, vrf_id,
                                       NSM_MSG_QOS_CLIENT_RELEASE,
                                       nbytes, &msg_id);


  /* Create 32 bit prefix */
  pal_mem_set (&p, 0, sizeof (struct prefix));
  p.family = AF_INET;
  p.prefixlen = IPV4_MAX_PREFIXLEN;
  PREP_FOR_NETWORK (p.u.prefix4.s_addr,resource_id, IPV4_MAX_PREFIXLEN);
  
  /* Get route node */
  rn = route_node_lookup (table, &p);
  if (rn && rn->info)
    {
      /*
       * Delete this resource and route node if free_resrc flag is set.
       */
      
      /* Get resource. */
      resource = rn->info;
      
      /* If resource needs to be freed, delete it, else unset
         the resource id. */
      if (free_resrc == 1)
        qos_common_resource_delete (resource);
      else
        resource->resource_id = 0;
      
      /* Set info to null */
      rn->info = NULL;
      
      /* Remove route node */
      route_unlock_node (rn);
      route_unlock_node (rn);
    }

  return QOS_OK;
}

int
qos_client_clean (struct nsm_client *nc, struct lib_globals *zg, 
                  u_int32_t protocol_id, u_int32_t ifindex) 
{
  int nbytes;
  struct nsm_client_handler *nch;
  u_char *pnt;
  u_int16_t size;
  struct nsm_msg_qos_clean msg;
  u_int32_t msg_id;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* Use sync. */
  nch = nc->async;

  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  pal_mem_set (&msg, 0, sizeof(struct nsm_msg_qos_clean));
  msg.protocol_id = protocol_id;
  if (ifindex)
    {
      NSM_SET_CTYPE (msg.cindex, NSM_QOS_CLEAN_CTYPE_IFINDEX);
      msg.ifindex = ifindex;
    }
  nbytes = nsm_encode_qos_clean (&pnt, &size, &msg);

  if (nbytes < 0)
    return nbytes;

  /* Send message. */
  nbytes =  nsm_client_send_message (nch, vr_id, vrf_id,
                                       NSM_MSG_QOS_CLIENT_CLEAN,
                                       nbytes, &msg_id);

  if (nbytes < 0)
    return nbytes;

  return 0;
}

#ifdef HAVE_GMPLS
int
gmpls_qos_client_release (struct nsm_client *nc,
                          struct lib_globals *zg, 
                          struct route_table *table,
                          u_int32_t resource_id,
                          u_int32_t protocol_id,
                          u_int32_t tl_gifindex,
                          u_int32_t dl_gifindex,
                          u_int8_t free_resrc)
{
  int nbytes;
  struct qos_resource *resource;
  struct route_node *rn;
  struct prefix p;
  struct nsm_client_handler *nch;
  u_char *pnt;
  u_int16_t size;
  struct nsm_msg_gmpls_qos_release gmpls_msg;
  u_int32_t msg_id;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* Use sync. */
  nch = nc->async;

  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  pal_mem_set (&gmpls_msg, 0, sizeof(struct nsm_msg_gmpls_qos_release));
  gmpls_msg.resource_id = resource_id;
  gmpls_msg.protocol_id = protocol_id;

  if (tl_gifindex)
    {
      NSM_SET_CTYPE (gmpls_msg.cindex, NSM_GMPLS_QOS_RELEASE_CTYPE_TL_GIFINDEX);
      gmpls_msg.tel_gifindex = tl_gifindex;
    }
  if (dl_gifindex)
    {
      NSM_SET_CTYPE (gmpls_msg.cindex, NSM_GMPLS_QOS_RELEASE_CTYPE_DL_GIFINDEX);
      gmpls_msg.dl_gifindex = dl_gifindex;
    }
  nbytes = nsm_encode_gmpls_qos_release (&pnt, &size, &gmpls_msg);

  if (nbytes < 0)
    return nbytes;

  /* Send message. */
  nbytes =  nsm_client_send_message (nch, vr_id, vrf_id,
                                     NSM_MSG_GMPLS_QOS_CLIENT_RELEASE,
                                     nbytes, &msg_id);

  /* Create 32 bit prefix */
  pal_mem_set (&p, 0, sizeof (struct prefix));
  p.family = AF_INET;
  p.prefixlen = IPV4_MAX_PREFIXLEN;
  PREP_FOR_NETWORK (p.u.prefix4.s_addr,resource_id, IPV4_MAX_PREFIXLEN);
  
  /* Get route node */
  rn = route_node_lookup (table, &p);
  if (rn && rn->info)
    {
      /*
       * Delete this resource and route node if free_resrc flag is set.
       */
      
      /* Get resource. */
      resource = rn->info;
      
      /* If resource needs to be freed, delete it, else unset
         the resource id. */
      if (free_resrc == 1)
        qos_common_resource_delete (resource);
      else
        resource->resource_id = 0;
      
      /* Set info to null */
      rn->info = NULL;
      
      /* Remove route node */
      route_unlock_node (rn);
      route_unlock_node (rn);
    }

  return QOS_OK;
}

int
gmpls_qos_client_clean (struct nsm_client *nc, struct lib_globals *zg, 
                        u_int32_t protocol_id, 
                        u_int32_t tl_gifindex, 
                        u_int32_t dl_gifindex) 
{
  int nbytes;
  struct nsm_client_handler *nch;
  u_char *pnt;
  u_int16_t size;
  struct nsm_msg_gmpls_qos_clean gmpls_msg;
  u_int32_t msg_id;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* Use sync. */
  nch = nc->async;

  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  pal_mem_set (&gmpls_msg, 0, sizeof(struct nsm_msg_gmpls_qos_clean));
  gmpls_msg.protocol_id = protocol_id;

  if (tl_gifindex)
    {
      NSM_SET_CTYPE (gmpls_msg.cindex, NSM_GMPLS_QOS_CLEAN_CTYPE_TL_GIFINDEX);
      gmpls_msg.tel_gifindex = tl_gifindex;
    }
  if (dl_gifindex)
    {
      NSM_SET_CTYPE (gmpls_msg.cindex, NSM_GMPLS_QOS_CLEAN_CTYPE_DL_GIFINDEX);
      gmpls_msg.dl_gifindex = dl_gifindex;
    }
  nbytes = nsm_encode_gmpls_qos_clean (&pnt, &size, &gmpls_msg);

  if (nbytes < 0)
    return nbytes;

  /* Send message. */
  nbytes =  nsm_client_send_message (nch, vr_id, vrf_id,
                                     NSM_MSG_GMPLS_QOS_CLIENT_CLEAN,
                                     nbytes, &msg_id);

  if (nbytes < 0)
    return nbytes;

  return 0;
}
#endif /*HAVE_GMPLS */

void
qos_client_deinit (struct lib_globals *zg, 
                   struct route_table **table,
                   u_char del_table)
{
  struct qos_resource *resource;
  struct route_node *rn;

  if (! table || ! *table)
    return;

  /* Delete route_table */
  for (rn = route_top (*table); rn; rn = route_next (rn))
    {
      /* Get resource. */
      if ((resource = rn->info) != NULL)
        {
          /* Delete data. */
          qos_common_resource_delete (resource);
          rn->info = NULL;
          route_unlock_node (rn);
        }
    }

  if (del_table)
    {
      /* Remove table as well */
      route_table_finish (*table);
      *table = NULL;
    }
}
#endif /* #ifdef HAVE_TE */
