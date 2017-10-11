/* Copyright (C) 2002-2003  All Rights Reserved. */

/* NSM protocol server implementation.  */
#include "pal.h"
#include "lib.h"
#include "avl_tree.h"
#include "modbmap.h"
#include "thread.h"
#include "vector.h"
#include "message.h"
#include "network.h"
#include "nsmd.h"
#include "nexthop.h"
#include "cli.h"
#ifdef HAVE_DSTE
#include "dste.h"
#endif /* HAVE_DSTE */
#include "nsm_message.h"
#include "log.h"
#include "table.h"
#include "ptree.h"

#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */

#ifdef HAVE_MPLS
#include "mpls.h"
#include "mpls_common.h"
#include "nsm_mpls.h"
#include "nsm_lp_serv.h"
#ifdef HAVE_MPLS_VC
#include "nsm_mpls_vc.h"
#endif /* HAVE_MPLS_VC */
#include "nsm_mpls_rib.h"
#endif /* HAVE_MPLS */

#ifdef HAVE_DSTE
#include "nsm_dste.h"
#endif /* HAVE_DSTE */

#ifdef HAVE_VPLS
#include "nsm_vpls.h"
#endif /* HAVE_VPLS */

#ifdef HAVE_MPLS_FRR
#include "mpls_common.h"
#include "mpls_client.h"
#endif /* HAVE_MPLS_FRR */

#include "nsm_server.h"
#ifdef HAVE_L3
#include "rib/nsm_server_rib.h"
#endif /* HAVE_L3 */
#include "nsm_router.h"
#include "nsm_debug.h"
#ifdef HAVE_L2
#include "nsm_bridge.h"
#include "nsm_pmirror.h"
#ifdef HAVE_VLAN
#include "nsm_vlan.h"
#endif /* HAVE_VLAN */
#endif /* HAVE_L2 */
#ifdef HAVE_AUTHD
#include "nsm_auth.h"
#endif /* HAVE_AUTHD */
#include "nsm_interface.h"
#ifdef HAVE_L3
#include "nsm_nexthop.h"
#include "rib.h"
#endif /* HAVE_L3 */
#ifdef HAVE_LACPD
#include "nsm_lacp.h"
#endif /* HAVE_LACPD */
#ifdef HAVE_ONMD
#include "nsm_oam.h"
#endif /* HAVE_ONMD */
#ifdef HAVE_ELMID
#include "nsm_elmi.h"
#endif /* HAVE_ELMID */

#ifdef HAVE_L3
#include "nsm_static_mroute.h"
#endif /* HAVE_L3 */

#ifdef HAVE_MCAST_IPV4
#include "lib_mtrace.h"
#include "nsm_mcast4.h"
#include "nsm/mcast/mcast4/nsm_mcast4_func.h"
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
#include "nsm_mcast6.h"
#include "nsm/mcast/mcast6/nsm_mcast6_func.h"
#endif /* HAVE_MCAST_IPV6 */

#ifdef HAVE_TE
#include "nsm_qos_serv.h"
#endif /* HAVE_TE */

#ifdef HAVE_MPLS
#include "nsm_lp_serv.h"
#include "nsm_mpls_rib.h"
#endif /* HAVE_MPLS */

#ifdef HAVE_MPLS_VC
#include "nsm_mpls_vc.h"
#ifdef HAVE_MS_PW
#include "nsm_mpls_ms_pw.h"
#endif /* HAVE_MS_PW */
#endif /* HAVE_MPLS_VC */

#if defined HAVE_MPLS_OAM || defined HAVE_MPLS_OAM_ITUT || defined HAVE_NSM_MPLS_OAM
#include "nsm_mpls_oam.h"
#endif /* HAVE_MPLS_OAM || HAVE_MPLS_OAM_ITUT || HAVE_NSM_MPLS_OAM */

#ifdef HAVE_GMPLS
#include "gmpls/gmpls.h"
#include "gmpls/nsm_gmpls_if.h"
#include "gmpls/nsm_gmpls_ifapi.h"
#endif /* HAVE_GMPLS */

#ifdef HAVE_VCCV
#include "mpls_util.h"
#endif /* HAVE_VCCV */

#ifdef HAVE_CRX
#include "crx_vip.h"
#include "crx_rib.h"
#endif /* HAVE_CRX */

#ifdef HAVE_RMM
#include "nsm_rd.h"
#endif /* HAVE_RMM */

#ifdef HAVE_RMOND
#include "nsm_rmon.h"
#endif

#ifdef HAVE_SMI
#include "lib_fm_api.h"
#include "smi_nsm_fm.h"
#endif /* HAVE_SMI */

#if defined HAVE_I_BEB || defined HAVE_B_BEB
#include "nsm_bridge_pbb.h"
#if defined HAVE_PBB_TE
#include "nsm_pbb_te.h"
#endif /* HAVE_PBB_TE */
#endif /* HAVE_I_BEB || HAVE_B_BEB */

#if defined HAVE_I_BEB || defined HAVE_B_BEB
#include "nsm_bridge_pbb.h"
#if defined HAVE_PBB_TE
#include "nsm_pbb_te.h"
#endif /* HAVE_PBB_TE */
#endif /* HAVE_I_BEB || HAVE_B_BEB */

#ifdef HAVE_BFD
void nsm_bfd_init (struct lib_globals *zg);
#endif /* HAVE_BFD */

#if defined HAVE_PBB_TE
#if defined HAVE_GMPLS && defined HAVE_GELS
#if defined HAVE_I_BEB && defined HAVE_B_BEB

extern int
nsm_pbb_te_request_label_with_name(u_char  *tesi_name,
                                   struct pbb_te_label* new_label,
                                   nsm_pbb_tesid_t* tesid);

extern int
nsm_pbb_te_request_label_with_rmac(u_char  *rmac, u_int16_t bvid,
                                   struct pbb_te_label* new_label,
                                   nsm_pbb_tesid_t* tesid);
extern int
nsm_pbb_te_release_label(nsm_pbb_tesid_t tesid, u_char *bmac, u_int16_t bvid);

extern int
nsm_pbb_te_beb_validate(nsm_pbb_tesid_t tesid, u_char *bmac, u_int16_t bvid);

#endif
extern int
nsm_pbb_te_action_validate(struct interface *ifp,
                           u_char *bmac, u_int16_t bvid);
#endif
#endif /* HAVE_PBB_TE */

#ifdef HAVE_GMPLS
void
nsm_server_control_adj_msg_set (struct control_adj *ca,
                                cindex_t cindex,
                                struct nsm_msg_control_adj *msg);
#endif /* HAVE_GMPLS */

s_int32_t
nsm_server_recv_ftn_gen_ipv4 (struct nsm_msg_header *header,
                              void *arg, void *message);

/* Set service type flag.  */
s_int32_t
nsm_server_set_service (struct nsm_server *ns, int service)
{
  if (service >= NSM_SERVICE_MAX)
    return NSM_ERR_INVALID_SERVICE;

  SET_FLAG (ns->service.bits, (1 << service));

  return 0;
}

/* Unset service type flag.  */
s_int32_t
nsm_server_unset_service (struct nsm_server *ns, int service)
{
  if (service >= NSM_SERVICE_MAX)
    return NSM_ERR_INVALID_SERVICE;

  UNSET_FLAG (ns->service.bits, (1 << service));

  return 0;
}

/* Check service type flag.  */
s_int32_t
nsm_service_check (struct nsm_server_entry *nse, int service)
{
  if (service >= NSM_SERVICE_MAX)
    return 0;

  return CHECK_FLAG (nse->service.bits, (1 << service));
}

/* Set parser function and call back function. */
void
nsm_server_set_callback (struct nsm_server *ns, int message_type,
                         NSM_PARSER parser, NSM_CALLBACK callback)
{
  if (message_type >= NSM_MSG_MAX)
    return;

  ns->parser[message_type] = parser;
  ns->callback[message_type] = callback;
}

/* Free NSM server entry.  */
void
nsm_server_entry_free (struct nsm_server_entry *nse)
{
  struct nsm_message_queue *queue;

  while ((queue
          = (struct nsm_message_queue *) FIFO_TOP (&nse->send_queue)) != NULL)
    {
      FIFO_DEL (queue);
      XFREE (MTYPE_NSM_MSG_QUEUE_BUF, queue->buf);
      XFREE (MTYPE_NSM_MSG_QUEUE, queue);
    }

  /* Cancel threads.  */
  THREAD_OFF (nse->t_ipv4);
#ifdef HAVE_IPV6
  THREAD_OFF (nse->t_ipv6);
#endif /* HAVE_IPV6 */
  THREAD_OFF (nse->t_write);

  XFREE (MTYPE_NSM_SERVER_ENTRY, nse);
}

/* Client connect to NSM server.  */
s_int32_t
nsm_server_connect (struct message_handler *ms, struct message_entry *me,
                    pal_sock_handle_t sock)
{
  struct nsm_server_entry *nse;

  nse = XCALLOC (MTYPE_NSM_SERVER_ENTRY, sizeof (struct nsm_server_entry));
  nse->send.len = NSM_MESSAGE_MAX_LEN;
  nse->recv.len = NSM_MESSAGE_MAX_LEN;
  nse->len_ipv4 = NSM_MESSAGE_MAX_LEN;
#ifdef HAVE_IPV6
  nse->len_ipv6 = NSM_MESSAGE_MAX_LEN;
#endif /* HAVE_IPV6 */

  me->info = nse;
  nse->me = me;
  nse->ns = ms->info;
  nse->connect_time = pal_time_sys_current (NULL);
  FIFO_INIT (&nse->send_queue);

  return 0;
}

s_int32_t
nsm_server_disconnect_vr (struct nsm_master *nm,
                          struct nsm_server_entry *nse,
                          struct nsm_server_client *nsc)
{
#if defined (HAVE_MCAST_IPV4) || defined (HAVE_MCAST_IPV6)
  struct apn_vrf *ivrf;
  struct nsm_vrf *nvrf;
  u_int32_t idx;
#endif /* (HAVE_MCAST_IPV4) || (HAVE_MCAST_IPV6) */

#ifdef HAVE_MPLS
  mpls_owner_t owner;
#endif /* HAVE_MPLS */

#ifdef HAVE_MPLS
  /* Clean up label pool.  */
  if (NSM_CAP_HAVE_MPLS)
    {
      if (nsm_restart_state(nse->service.protocol_id))
        nsm_lp_server_set_stale_for (nm, nse->service.protocol_id);
      else
        nsm_lp_server_clean_in_use_for (nm, nse->service.protocol_id);
    }
#endif /* HAVE_MPLS */

#ifdef HAVE_TE
  /* Cleanup QOS resources. */
  nsm_qos_serv_clean_for (nm, nse->service.protocol_id, 0);
#endif /* HAVE_TE */

#ifdef HAVE_L3
  /* Clean up nexthop registrations for this client. */
  nsm_nexthop_reg_clean (nm, nsc);

  /* Clean up multicast nexthop registrations */
  if (APN_PROTO_MCAST (nse->service.protocol_id))
    nsm_multicast_nh_reg_client_disconnect (nm, nsc);
#endif /* HAVE_L3 */

#ifdef HAVE_MPLS
  if (NSM_CAP_HAVE_MPLS)
    {
      /* Cleanup mpls rib. */
      if (nm->nmpls)
        {
          owner = gmpls_proto_to_owner (nse->service.protocol_id);
          if (owner != MPLS_UNKNOWN)
            nsm_gmpls_rib_clean_client (nm, owner, nse->service.protocol_id);

#ifdef HAVE_VPLS
          if (owner == MPLS_LDP)
            nsm_vpls_fib_cleanup_all (nm);
#endif /* HAVE_VPLS */
        }
    }
#endif /* HAVE_MPLS */

#if defined (HAVE_MCAST_IPV4) || defined (HAVE_MCAST_IPV6)
  if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_IPV4_MRIB)
      || NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_IPV6_MRIB))
    VECTOR_LOOP (nm->vr->vrf_vec, ivrf, idx)
      if ((nvrf = ivrf->proto))
        {
#ifdef HAVE_MCAST_IPV4
          nsm_mcast_client_disconnect (nvrf, nse, nsc);
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
          nsm_mcast6_client_disconnect (nvrf, nse, nsc);
#endif /* HAVE_MCAST_IPV6 */
        }
#endif /* (HAVE_MCAST_IPV4) || (HAVE_MCAST_IPV6) */

#ifdef HAVE_LACPD
  if (nse->service.protocol_id == APN_PROTO_LACP)
    nsm_lacp_delete_all_aggregators (nm);
#endif /* HAVE_LACPD */

  return 0;
}

/* Client disconnect from NSM server.  This function should not be
   called directly.  Message library call this.  */
s_int32_t
nsm_server_disconnect (struct message_handler *ms, struct message_entry *me,
                       pal_sock_handle_t sock)
{
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct nsm_server_client *nsc;
  struct nsm_redistribute *redist;
  struct nsm_redistribute *next;
  struct nsm_vrf *vrf;
  vector vec;
  struct apn_vr *vr;
  struct route_node *rn;
  struct interface *ifp;
  u_int32_t i;

  nse = me->info;
  ns = nse->ns;

  /* Figure out NSM client which nse belongs to.  */
  nsc = nse->nsc;
  if (nsc == NULL)
    {
      if (IS_NSM_DEBUG_EVENT)
        zlog_info (NSM_ZG, "NSM client disconnect");

      return 0;
    }

  /* Logging.  */
  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "NSM client ID %d disconnect", nse->service.client_id);

#ifdef HAVE_SMI
  /* FM recording */
  smi_record_fault (nzg, FM_GID_NSM,
                    NSM_SMI_ALARM_NSM_CLIENT_SOCKET_DISCONNECT,
                    __LINE__, __FILE__, &(nse->service.client_id));
#endif /* HAVE_SMI */

#ifdef HAVE_L3
  /* Restart state turns on */
  if (nsm_restart_time (nse->service.protocol_id))
    nsm_restart_state_set (nse->service.protocol_id);

  /* Inform ISIS, BGP is down */
  if (nse->service.protocol_id == APN_PROTO_BGP)
    nsm_server_send_bgp_down (nse);

  /* Inform IGP that LDP is down, unless it is undergoing graceful restart */
  if (nse->service.protocol_id == APN_PROTO_LDP) 
    nsm_server_send_ldp_down (nse, ns->zg->vr_instance, 0);

#endif /* HAVE_L3 */

  for (i = 0; i < vector_max (nzg->vr_vec); i++)
    if ((vr = vector_slot (nzg->vr_vec, i)))
      nsm_server_disconnect_vr (vr->proto, nse, nsc);

#ifdef HAVE_L3
  /* Clean up RIB.  */
  if (nsm_service_check (nse, NSM_SERVICE_ROUTE))
    nsm_rib_clean_client (nse->service.client_id, nse->service.protocol_id);
#endif /* HAVE_L3 */

  /* Unset redistribute information.  */
  for (redist = nse->redist; redist; redist = next)
    {
      next = redist->next;
      nm = nsm_master_lookup_by_id (nzg, redist->vr_id);
      if (nm != NULL)
        {
          vrf = nsm_vrf_lookup_by_id (nm, redist->vrf_id);
          if (vrf != NULL)
            {
              if (redist->afi == 0 || redist->afi == AFI_IP)
                {
                  vec = vrf->afi[AFI_IP].redist[redist->type];
                  for (i = 0; i < vector_max (vec); i++)
                    if (vector_slot (vec, i) == nse)
                      vector_unset (vec, i);
                }

#ifdef HAVE_IPV6
              if (redist->afi == 0 || redist->afi == AFI_IP6)
                {
                  vec = vrf->afi[AFI_IP6].redist[redist->type];
                  for (i = 0; i < vector_max (vec); i++)
                    if (vector_slot (vec, i) == nse)
                      vector_unset (vec, i);
                }
#endif /* HAVE_IPV6 */
            }
           /* Flush client from clean up response pending interface list. */
           for (rn = route_top (nm->vr->ifm.if_table); rn; rn = route_next (rn))
             if ((ifp = rn->info))
               nsm_server_if_clean_list_remove(ifp, nse);
        }
      XFREE (MTYPE_NSM_REDISTRIBUTE, redist);
    }
  nm = nsm_master_lookup_by_id (nzg, 0);
  if (nm == NULL)
    return 0;

#ifdef HAVE_NSM_MPLS_OAM
  nsm_mpls_client_disconnect (nm, nse);
#endif /* HAVE_NSM_MPLS_OAM */

  /* Remove nse from linked list.  */
  if (nse->prev)
    nse->prev->next = nse->next;
  else
    nsc->head = nse->next;
  if (nse->next)
    nse->next->prev = nse->prev;
  else
    nsc->tail = nse->prev;

#ifdef HAVE_MCAST_IPV4
  if (nsc->nsc_mc4_stats.blocks)
    XFREE (MTYPE_NSM_MCAST_STAT_BLOCK, nsc->nsc_mc4_stats.blocks);
#endif /* HAVE_MCAST_IPV4 */
#ifdef HAVE_MCAST_IPV6
  if (nsc->nsc_mc6_stats.blocks)
    XFREE (MTYPE_NSM_MCAST6_STAT_BLOCK, nsc->nsc_mc6_stats.blocks);
#endif /* HAVE_MCAST_IPV4 */

  /* If NSM server client becomes empty free it.  */
  if (nsc->head == NULL && nsc->tail == NULL)
    {
      vector_unset (ns->client, nsc->client_id);
      XFREE (MTYPE_NSM_SERVER_CLIENT, nsc);
    }

  /* Free NSM server entry.  */
  nsm_server_entry_free (nse);
  me->info = NULL;

  return 0;
}

/* De-queue NSM message.  */
s_int32_t
nsm_server_dequeue (struct thread *t)
{
  struct nsm_server_entry *nse;
  struct nsm_message_queue *queue;
  pal_sock_handle_t sock;
  s_int32_t nbytes;

  nse = THREAD_ARG (t);
  sock = THREAD_FD (t);
  nse->t_write = NULL;

  queue = (struct nsm_message_queue *) FIFO_HEAD (&nse->send_queue);
  if (queue)
    {
      nbytes = pal_sock_write (sock, queue->buf + queue->written,
                               queue->length - queue->written);
      if (nbytes <= 0)
        {
          if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
            {
              zlog_err (NSM_ZG, "NSM message send error socket %d %s",
                        nse->me->sock, pal_strerror(errno));
              return -1;
            }
        }
      else if (nbytes != (queue->length - queue->written))
        {
          queue->written += nbytes;
        }
      else
        {
          FIFO_DEL (queue);
          XFREE (MTYPE_NSM_MSG_QUEUE_BUF, queue->buf);
          XFREE (MTYPE_NSM_MSG_QUEUE, queue);
        }
    }

  if (FIFO_TOP (&nse->send_queue))
    THREAD_WRITE_ON (NSM_ZG, nse->t_write, nsm_server_dequeue, nse,
                     nse->me->sock);

  return 0;
}

/* Could not write the message to the socket, enqueue the message.  */
void
nsm_server_enqueue (struct nsm_server_entry *nse, u_char *buf,
                    u_int16_t length, u_int16_t written)
{
  struct nsm_message_queue *queue;

  queue = XCALLOC (MTYPE_NSM_MSG_QUEUE, sizeof (struct nsm_message_queue));
  if(queue == NULL)
    {
      zlog_err (NSM_ZG, "Memory for NSM message queue is exhausted");
      THREAD_WRITE_ON (NSM_ZG, nse->t_write, nsm_server_dequeue, nse,
                       nse->me->sock);
      return;
    }

  queue->buf = XMALLOC (MTYPE_NSM_MSG_QUEUE_BUF, length);
  if(queue->buf == NULL)
    {
      zlog_err (NSM_ZG, "Memory for NSM message queue buffer is exhausted");
      THREAD_WRITE_ON (NSM_ZG, nse->t_write, nsm_server_dequeue, nse,
                       nse->me->sock);
      return;
    }
  pal_mem_cpy (queue->buf, buf, length);
  queue->length = length;
  queue->written = written;

  FIFO_ADD (&nse->send_queue, queue);

  THREAD_WRITE_ON (NSM_ZG, nse->t_write, nsm_server_dequeue, nse,
                   nse->me->sock);
}

/* Send message to the client.  */
s_int32_t
nsm_server_send_message (struct nsm_server_entry *nse,
                         u_int32_t vr_id, vrf_id_t vrf_id,
                         s_int32_t type, u_int32_t msg_id, u_int16_t len)
{
  struct nsm_msg_header header;
  u_int16_t total_len;
  s_int32_t nbytes;

#ifdef HAVE_HA
  if ((HA_IS_STANDBY (nzg))
      && (! HA_ALLOW_NSM_MSG_TYPE(type)))
    return 0;
#endif /* HAVE_HA */

  nse->send.pnt = nse->send.buf;
  nse->send.size = nse->send.len;

  /* Prepare NSM message header.  */
  header.vr_id = vr_id;
  header.vrf_id = vrf_id;
  header.type = type;
  header.length = len + NSM_MSG_HEADER_SIZE;
  header.message_id = msg_id;
  total_len = len + NSM_MSG_HEADER_SIZE;

  nsm_encode_header (&nse->send.pnt, &nse->send.size, &header);

  if (FIFO_TOP (&nse->send_queue))
    {
      nsm_server_enqueue (nse, nse->send.buf, total_len, 0);
      return 0;
    }

  /* Send message.  */
  nbytes = pal_sock_write (nse->me->sock, nse->send.buf, total_len);
  if (nbytes <= 0)
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        nsm_server_enqueue (nse, nse->send.buf, total_len, 0);
      else
        {
          zlog_err (NSM_ZG, "NSM message send error socket %d %s",
                    nse->me->sock, pal_strerror(errno));
          return -1;
        }
    }
  else if (nbytes != total_len)
    nsm_server_enqueue (nse, nse->send.buf, total_len, nbytes);
  else
    {
      nse->last_write_type = type;
      nse->send_msg_count++;
    }
  return 0;
}

/* Send message to the client with specific message id.  */
s_int32_t
nsm_server_send_message_msgid (struct nsm_server_entry *nse,
                               u_int32_t vr_id, vrf_id_t vrf_id,
                               s_int32_t type, u_int32_t *msg_id, u_int16_t len)
{
  u_int32_t mid;

  /* If msg_id is NULL or 0, use the message_id counter in nse */
  if ((msg_id == NULL) || (*msg_id == 0))
    {
      ++nse->message_id;

      if (nse->message_id == 0)
        nse->message_id = 1;

      mid = nse->message_id;

      if (msg_id != NULL)
        *msg_id = mid;
    }
  else
    mid = *msg_id;

  return nsm_server_send_message (nse, vr_id, vrf_id, type, mid, len);
}

/* Send service message to NSM client.  */
s_int32_t
nsm_server_send_service (struct nsm_server_entry *nse, u_int32_t msg_id,
                         struct nsm_msg_service *service)
{
  s_int32_t len;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_service (&nse->send.pnt, &nse->send.size, service);
  if (len < 0)
    return len;

  /* Send it to client.  */
  nsm_server_send_message (nse, vr_id, vrf_id,
                           NSM_MSG_SERVICE_REPLY, msg_id, len);

  return 0;
}

/* Check the interface is bind to VRF or VC.  */
s_int32_t
nsm_server_if_bind_check (struct nsm_server_entry *nse, struct interface *ifp)
{
  /* When interface is not valid.  */
  if (! ifp)
    return 0;

#ifdef HAVE_SUPPRESS_UNMAPPED_IF
  if (! CHECK_FLAG (ifp->status, NSM_INTERFACE_MAPPED))
    return 0;
#endif /* HAVE_SUPPRESS_UNMAPPED_IF */

#ifdef HAVE_INTERFACE_MANAGE
  if (CHECK_FLAG (ifp->status, NSM_INTERFACE_MANAGE))
    return 0;
#endif /* HAVE_INTERFACE_MANAGE */

#ifdef HAVE_MPLS_VC
  /* Check VC interface binding.  */
  if (! nsm_service_check (nse, NSM_SERVICE_MPLS_VC)
      && (NSM_IF_BIND_CHECK (ifp, MPLS_VC) ||
          NSM_IF_BIND_CHECK (ifp, MPLS_VC_VLAN)))
    return 0;
#endif /* HAVE_MPLS_VC */

  /* Check VRF interface binding.  */
  if (! nsm_service_check (nse, NSM_SERVICE_VRF)
      && NSM_IF_BIND_CHECK (ifp, VRF))
    return 0;

#ifdef HAVE_VPLS
  /* Check VPLS interface binding.  */
  if (! nsm_service_check (nse, NSM_SERVICE_VPLS)
      && (NSM_IF_BIND_CHECK (ifp, VPLS) ||
          NSM_IF_BIND_CHECK (ifp, VPLS_VLAN)))
    return 0;
#endif /* HAVE_VPLS */

  return 1;
}

/* Send interface information to client.  */
s_int32_t
nsm_server_send_link_add (struct nsm_server_entry *nse,
                          struct interface *ifp)
{
  struct nsm_msg_link msg;
  s_int32_t len;

  /* Check requested service.  */
  if (! nsm_service_check (nse, NSM_SERVICE_INTERFACE))
    return 0;

#ifdef HAVE_INTERFACE_MANAGE
  if (CHECK_FLAG (ifp->status, NSM_INTERFACE_MANAGE))
    return 0;
#endif /* HAVE_INTERFACE_MANAGE */

  /* Init Cindex. */
  msg.cindex = 0;

  /* Interface index is mandatory.  */
  msg.ifindex = ifp->ifindex;

  /* Interface Name TLV. */
  NSM_SET_CTYPE (msg.cindex, NSM_LINK_CTYPE_NAME);
  msg.ifname_len = INTERFACE_NAMSIZ;
  msg.ifname = ifp->name;

  /* Interface Flags TLV. */
  NSM_SET_CTYPE (msg.cindex, NSM_LINK_CTYPE_FLAGS);
  msg.flags = ifp->flags;

#ifdef HAVE_ONMD
  if (nsm_service_check (nse, NSM_SERVICE_INTERFACE) &&
      nse->service.protocol_id != APN_PROTO_ONM)
    {
      /* Initially when the link was disconnected and protocol
       * module is coming up NSM_MSG_LINK_ADD is unsetting the IFF_UP
       * flag because of which as per ONM the link is shutdown.
       * As ONM needs the current link status the following is being done */
      if (! if_is_running (ifp))
        msg.flags &= ~IFF_UP;
    }
#else
  if (! if_is_running (ifp))
    msg.flags &= ~IFF_UP;
#endif /* HAVE_ONMD */

  /* Interface status TLV.  */
  NSM_SET_CTYPE (msg.cindex, NSM_LINK_CTYPE_STATUS);
  msg.status = ifp->status;

  /* Interface Metric TLV. */
  NSM_SET_CTYPE (msg.cindex, NSM_LINK_CTYPE_METRIC);
  msg.metric = ifp->metric;

  /* Interface MTU TLV. */
  NSM_SET_CTYPE (msg.cindex, NSM_LINK_CTYPE_MTU);
  msg.mtu = ifp->mtu;

  /* Hardware Address TLV. */
  NSM_SET_CTYPE (msg.cindex, NSM_LINK_CTYPE_HW_ADDRESS);
  msg.hw_type = ifp->hw_type;
  msg.hw_addr_len = ifp->hw_addr_len;
  if (ifp->hw_addr_len)
    pal_mem_cpy (msg.hw_addr, ifp->hw_addr, ifp->hw_addr_len);

  /* Interface bandwidth TLV. */
  NSM_SET_CTYPE (msg.cindex, NSM_LINK_CTYPE_BANDWIDTH);
  pal_mem_cpy (&msg.bandwidth, &ifp->bandwidth, 4);

  /* Interface duplex TLV. */
  NSM_SET_CTYPE (msg.cindex, NSM_LINK_CTYPE_DUPLEX);
  msg.duplex = ifp->duplex;

#ifdef HAVE_TE
  /* Admin Group TLV. */
  NSM_SET_CTYPE (msg.cindex, NSM_LINK_CTYPE_ADMIN_GROUP);
  msg.admin_group = ifp->admin_group;

  /* Max reservable bandwidth TLV. */
  NSM_SET_CTYPE (msg.cindex, NSM_LINK_CTYPE_MAX_RESV_BANDWIDTH);
  pal_mem_cpy (&msg.max_resv_bw, &ifp->max_resv_bw, sizeof (float32_t));

#ifdef HAVE_DSTE
  /* BW MODE TLV. */
  NSM_SET_CTYPE (msg.cindex, NSM_LINK_CTYPE_BC_MODE);
  msg.bc_mode = ifp->bc_mode;
  NSM_SET_CTYPE (msg.cindex, NSM_LINK_CTYPE_BW_CONSTRAINT);
  pal_mem_cpy (msg.bw_constraint, ifp->bw_constraint,
               MAX_BW_CONST * sizeof (float32_t));
#endif /* HAVE_DSTE */
#endif /* HAVE_TE */
#ifdef HAVE_MPLS
  IF_NSM_CAP_HAVE_MPLS
    {
      NSM_SET_CTYPE (msg.cindex, NSM_LINK_CTYPE_LABEL_SPACE);
      msg.lp = ifp->ls_data;
    }
#endif /* HAVE_MPLS */

  NSM_SET_CTYPE (msg.cindex, NSM_LINK_CTYPE_BINDING);
  msg.bind = ifp->bind;

#ifdef HAVE_VRX
  NSM_SET_CTYPE (msg.cindex, NSM_LINK_CTYPE_VRX_FLAG);
  msg.vrx_flag = ifp->vrx_flag;

  NSM_SET_CTYPE (msg.cindex, NSM_LINK_CTYPE_VRX_FLAG_LOCALSRC);
  msg.local_flag = ifp->local_flag;
#endif /* HAVE_VRX */

#ifdef HAVE_LACPD
  NSM_SET_CTYPE (msg.cindex, NSM_LINK_CTYPE_LACP_KEY);
  msg.lacp_admin_key = ifp->lacp_admin_key;
  msg.agg_param_update = ifp->agg_param_update;
#endif /* HAVE_LACPD */

#ifdef HAVE_GMPLS
  /* GMPLS Interface index  */
  msg.gifindex = ifp->gifindex;
  NSM_SET_CTYPE (msg.cindex, NSM_LINK_CTYPE_GMPLS_TYPE);
  msg.gtype = ifp->gmpls_type;
#endif /* HAVE_GMPLS */

  /* Debug */
  if (IS_NSM_DEBUG_SEND)
    {
      zlog_info (nse->ns->zg, "NSM SEND link message");
      nsm_interface_dump (nse->ns->zg, &msg);
    }

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_link (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  /* Send it to client.  */
  nsm_server_send_message (nse, 0, 0, NSM_MSG_LINK_ADD, 0, len);

  return 0;
}

/* Send interface information to client.  */
s_int32_t
nsm_server_send_link_delete (struct nsm_server_entry *nse,
                             struct interface *ifp,
                             s_int32_t logical_deletion)
{
  struct nsm_msg_link msg;
  s_int32_t len = 0;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_link));

  /* Check requested service.  */
  if (! nsm_service_check (nse, NSM_SERVICE_INTERFACE))
    return 0;

#ifdef HAVE_INTERFACE_MANAGE
  if (CHECK_FLAG (ifp->status, NSM_INTERFACE_MANAGE))
    return 0;
#endif /* HAVE_INTERFACE_MANAGE */

  /* Init Cindex. */
  msg.cindex = 0;

  /* Interface index is mandatory.  */
  msg.ifindex = ifp->ifindex;

#ifdef HAVE_GMPLS
  /* Interface index is mandatory.  */
  msg.gifindex = ifp->gifindex;
#endif /*HAVE_GMPLS*/

  /* Interface Name TLV. */
  NSM_SET_CTYPE (msg.cindex, NSM_LINK_CTYPE_NAME);
  msg.ifname_len = INTERFACE_NAMSIZ;
  msg.ifname = ifp->name;

  len = nsm_encode_link (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  nsm_server_send_message (nse, 0, 0, NSM_MSG_LINK_DELETE, 0, len);

  /* If interface still present physically, done. */
  if(logical_deletion)
    return 0;

  /* Add clean-up response pending to interface list. */
  nsm_server_if_clean_list_add(ifp, nse);

  return 0;
}

s_int32_t
nsm_server_send_link_update (struct nsm_server_entry *nse,
                             struct interface *ifp, cindex_t cindex)
{

  struct nsm_msg_link msg;
#ifdef HAVE_L3
  struct connected *ifc;
#endif /* HAVE_L3 */
  s_int32_t len;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_link));

  /* Check requested service.  */
  if (!nsm_service_check (nse, NSM_SERVICE_INTERFACE))
    return 0;

  /* Init Cindex. */
  msg.cindex = cindex;

  /* Interface index is mandatory.  */
  msg.ifindex = ifp->ifindex;

#ifdef HAVE_GMPLS
  /* Interface index is mandatory.  */
  msg.gifindex = ifp->gifindex;
#endif /*HAVE_GMPLS*/

  /* Interface Name TLV is mandatory.  */
  NSM_SET_CTYPE (msg.cindex, NSM_LINK_CTYPE_NAME);
  msg.ifname_len = INTERFACE_NAMSIZ;
  msg.ifname = ifp->name;

  /* Interface Flags TLV. */
  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_FLAGS))
    {
      msg.flags = ifp->flags;
      if (! if_is_running (ifp))
        msg.flags &= ~IFF_UP;
    }

  /* Interface status TLV.  */
  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_STATUS))
    msg.status = ifp->status;

  /* Interface Metric TLV.  */
  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_METRIC))
    msg.metric = ifp->metric;

  /* Interface MTU TLV. */
  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_MTU))
    msg.mtu = ifp->mtu;

  /* Hardware Address TLV.  */
  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_HW_ADDRESS))
    {
      msg.hw_type = ifp->hw_type;
      msg.hw_addr_len = ifp->hw_addr_len;
      if (ifp->hw_addr_len)
        pal_mem_cpy (msg.hw_addr, ifp->hw_addr, ifp->hw_addr_len);
    }

  /* Interface bandwidth TLV.  */
  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_BANDWIDTH))
    pal_mem_cpy (&msg.bandwidth, &ifp->bandwidth, 4);

#ifdef HAVE_TE
  /* Admin Group TLV.  */
  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_ADMIN_GROUP))
    msg.admin_group = ifp->admin_group;

  /* Max reservable bandwidth TLV.  */
  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_MAX_RESV_BANDWIDTH))
    pal_mem_cpy (&msg.max_resv_bw, &ifp->max_resv_bw, sizeof (float32_t));

#ifdef HAVE_DSTE
  /* BW MODE TLV.  */
  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_BC_MODE))
    msg.bc_mode = ifp->bc_mode;

  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_BW_CONSTRAINT))
    pal_mem_cpy (msg.bw_constraint, ifp->bw_constraint,
                 MAX_BW_CONST * sizeof (float32_t));
#endif /* HAVE_DSTE */
#endif /* HAVE_TE */

#ifdef HAVE_MPLS
  IF_NSM_CAP_HAVE_MPLS
    if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_LABEL_SPACE))
      msg.lp = ifp->ls_data;
#endif /* HAVE_MPLS */

#ifdef HAVE_VPLS
  if (CHECK_FLAG (ifp->bind, NSM_IF_BIND_VPLS) ||
                        CHECK_FLAG (ifp->bind, NSM_IF_BIND_VPLS_VLAN))
    if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_BINDING))
      msg.bind = ifp->bind;
#endif /* HAVE_VPLS */

#ifdef HAVE_VRX
  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_VRX_FLAG))
    msg.vrx_flag = ifp->vrx_flag;

  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_VRX_FLAG_LOCALSRC))
    msg.local_flag = ifp->local_flag;
#endif /* HAVE_VRX */

#ifdef HAVE_LACPD
  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_LACP_KEY))
    {
      msg.lacp_admin_key = ifp->lacp_admin_key;
      msg.agg_param_update = ifp->agg_param_update;
    }

#endif /* HAVE_LACPD */

#ifdef HAVE_L3
  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_NW_ADDRESS))
    {
      ifc = nsm_if_connected_primary_lookup (ifp);
      if (ifc)
        pal_mem_cpy (&msg.nw_addr, &ifc->address->u.prefix4,
                     sizeof (struct pal_in4_addr));
      else
        msg.nw_addr.s_addr = pal_ntoh32 (0x00000000);
    }
#endif /* HAVE_L3 */

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_link (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  /* Send it to client.  */
  nsm_server_send_message (nse, 0, 0, NSM_MSG_LINK_ATTR_UPDATE, 0, len);

  return 0;
}

/* VR bind/unbind are done outside of context.  Hence VR-ID = 0. */
s_int32_t
nsm_server_send_vr_bind (struct nsm_server_entry *nse,
                         struct interface *ifp)
{
  struct nsm_msg_vr_bind msg;
  s_int32_t len;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  msg.vr_id = ifp->vr->id;
  msg.ifindex = ifp->ifindex;

  len = nsm_encode_vr_bind (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  nsm_server_send_message (nse, vr_id, vrf_id, NSM_MSG_VR_BIND, 0, len);

  return 0;
}

s_int32_t
nsm_server_send_vr_unbind (struct nsm_server_entry *nse,
                           struct interface *ifp)
{
  struct nsm_msg_vr_bind msg;
  s_int32_t len;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  msg.vr_id = ifp->vr->id;
  msg.ifindex = ifp->ifindex;

  len = nsm_encode_vr_bind (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  nsm_server_send_message (nse, vr_id, vrf_id, NSM_MSG_VR_UNBIND, 0, len);

  return 0;
}

s_int32_t
nsm_server_send_vrf_bind (struct nsm_server_entry *nse,
                          struct interface *ifp)
{
  struct nsm_msg_vrf_bind msg;
  s_int32_t len;
  vrf_id_t vrf_id = 0;

  /* Check interface binding. */
  if (! nsm_server_if_bind_check (nse, ifp))
    return 0;

  msg.vrf_id = ifp->vrf->id;
  msg.ifindex = ifp->ifindex;

  len = nsm_encode_vrf_bind (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  nsm_server_send_message (nse, ifp->vr->id, vrf_id, NSM_MSG_VRF_BIND, 0, len);

  return 0;
}

s_int32_t
nsm_server_send_vrf_unbind (struct nsm_server_entry *nse,
                            struct interface *ifp)
{
  struct nsm_msg_vrf_bind msg;
  s_int32_t len;
  vrf_id_t vrf_id = 0;

  /* Check interface binding.  */
  if (! nsm_server_if_bind_check (nse, ifp))
    return 0;

  msg.vrf_id = ifp->vrf->id;
  msg.ifindex = ifp->ifindex;

  len = nsm_encode_vrf_bind (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  nsm_server_send_message (nse, ifp->vr->id, vrf_id,
                           NSM_MSG_VRF_UNBIND, 0, len);

  /* Del VIF locally as MCast protos assume this is done */

  return 0;
}

s_int32_t
nsm_server_send_interface_add (struct nsm_server_entry *nse,
                               struct interface *ifp)
{
  s_int32_t ret;

#ifdef HAVE_INTERFACE_MANAGE
  if (CHECK_FLAG (ifp->status, NSM_INTERFACE_MANAGE))
    return 0;
#endif /* HAVE_INTERFACE_MANAGE */

  /* Send link information.  */
  ret = nsm_server_send_link_add (nse, ifp);
  if (ret < 0)
    return ret;

  /* Send VR binding information.  */
  ret = nsm_server_send_vr_bind (nse, ifp);
  if (ret < 0)
    return ret;

  /* Send VRF binding information.  */
  if (nsm_service_check (nse, NSM_SERVICE_VRF))
    if ((ret = nsm_server_send_vrf_bind (nse, ifp)) < 0)
      return ret;

  return ret;
}

#ifndef HAVE_IMI
s_int32_t
nsm_server_send_user_update (struct nsm_server_entry *nse,
                             struct host_user *user, u_int32_t vr_id)
{
  struct nsm_msg_user msg;
  s_int32_t len;

  /* Initialize the message buffer.  */
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_user));

  msg.privilege = user->privilege;

  msg.username_len = pal_strlen (user->name) + 1;
  msg.username = user->name;

  if (user->password)
    {
      NSM_SET_CTYPE (msg.cindex, NSM_USER_CTYPE_PASSWORD);
      msg.password_len = pal_strlen (user->password) + 1;
      msg.password = user->password;
    }

  if (user->password_encrypt)
    {
      NSM_SET_CTYPE (msg.cindex, NSM_USER_CTYPE_PASSWORD_ENCRYPT);
      msg.password_encrypt_len = pal_strlen (user->password_encrypt) + 1;
      msg.password_encrypt = user->password_encrypt;
    }

  len = nsm_encode_user (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  nsm_server_send_message (nse, vr_id, 0, NSM_MSG_USER_UPDATE, 0, len);

  return 0;
}

s_int32_t
nsm_server_send_user_delete (struct nsm_server_entry *nse,
                             struct host_user *user, u_int32_t vr_id)
{
  struct nsm_msg_user msg;
  s_int32_t len;

  /* Initialize the message buffer.  */
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_user));

  msg.privilege = user->privilege;

  msg.username_len = pal_strlen (user->name) + 1;
  msg.username = user->name;

  len = nsm_encode_user (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  nsm_server_send_message (nse, vr_id, 0, NSM_MSG_USER_DELETE, 0, len);

  return 0;
}

s_int32_t
nsm_server_user_update_callback (struct apn_vr *vr, struct host_user *user)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  s_int32_t i;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    nsm_server_send_user_update (nse, user, vr->id);

  return 0;
}

s_int32_t
nsm_server_user_delete_callback (struct apn_vr *vr, struct host_user *user)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  s_int32_t i;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    nsm_server_send_user_delete (nse, user, vr->id);

  return 0;
}

#endif /* HAVE_IMI */

/* Send interface status information to client.  */
s_int32_t
nsm_server_send_interface_state (struct nsm_server_entry *nse,
                                 struct interface *ifp, int up,
                                 cindex_t cindex)
{
  struct nsm_msg_link msg;
  s_int32_t len;

  /* Check requested service.  */
  if (! nsm_service_check (nse, NSM_SERVICE_INTERFACE))
    return 0;

  /* Check interface binding.  */
  if (! nsm_server_if_bind_check (nse, ifp))
    return 0;

#if 0
  /* Check interface membership. */
  if (! NSM_INTERFACE_CHECK_MEMBERSHIP (ifp, nse->service.protocol_id))
    return 0;
#endif

  /* Interface index is mandatory.  */
  msg.ifindex = ifp->ifindex;

#ifdef HAVE_GMPLS
  /* Interface index is mandatory.  */
  msg.gifindex = ifp->gifindex;
#endif /*HAVE_GMPLS*/

  /* General information.  */
  msg.cindex = cindex;

  NSM_SET_CTYPE (msg.cindex, NSM_LINK_CTYPE_FLAGS);
  msg.flags = ifp->flags;
  if(!up)
    msg.flags &= ~IFF_UP;

  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_STATUS))
    msg.status = ifp->status;
  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_NAME))
    msg.ifname = ifp->name;
  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_METRIC))
    msg.metric = ifp->metric;
  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_MTU))
    msg.mtu = ifp->mtu;
  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_HW_ADDRESS))
    {
      msg.hw_type = ifp->hw_type;
      msg.hw_addr_len = ifp->hw_addr_len;
      if (ifp->hw_addr_len)
        pal_mem_cpy (msg.hw_addr, ifp->hw_addr, ifp->hw_addr_len);
    }
  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_BANDWIDTH))
    msg.bandwidth = ifp->bandwidth;
  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_DUPLEX))
    msg.duplex = ifp->duplex;
#ifdef HAVE_TE
  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_ADMIN_GROUP))
    msg.admin_group = ifp->admin_group;
  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_MAX_RESV_BANDWIDTH))
    pal_mem_cpy (&msg.max_resv_bw, &ifp->max_resv_bw,
                 sizeof (float32_t));
#ifdef HAVE_DSTE
  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_BC_MODE))
    msg.bc_mode = ifp->bc_mode;
  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_BW_CONSTRAINT))
    pal_mem_cpy (msg.bw_constraint, ifp->bw_constraint,
                 MAX_BW_CONST * sizeof (float32_t));
#endif /* HAVE_DSTE */
#endif /* HAVE_TE */

#ifdef HAVE_MPLS
  IF_NSM_CAP_HAVE_MPLS
    {
      if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_LABEL_SPACE))
        msg.lp = ifp->ls_data;
    }
#endif /* HAVE_MPLS */

  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_BINDING))
    msg.bind = ifp->bind;

#ifdef HAVE_VRX
  msg.vrx_flag = ifp->vrx_flag;
  msg.local_flag = ifp->local_flag;
#endif /* HAVE_VRX */

#ifdef HAVE_LACPD
  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_LACP_KEY))
    {
      msg.lacp_admin_key = ifp->lacp_admin_key;
      msg.agg_param_update = ifp->agg_param_update;
    }
#endif /* HAVE_LACPD */

#ifdef HAVE_GMPLS
  if (NSM_CHECK_CTYPE (msg.cindex, NSM_LINK_CTYPE_GMPLS_TYPE))
    msg.gtype = ifp->gmpls_type;
#endif /* HAVE_GMPLS */

  /* Debug */
  if (IS_NSM_DEBUG_SEND)
    {
      zlog_info (nse->ns->zg, "NSM SEND link message");
      nsm_interface_dump (nse->ns->zg, &msg);
    }

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_link (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  /* Send it to client.  */
  if (up)
    nsm_server_send_message (nse, ifp->vr->id, ifp->vrf->id,
                             NSM_MSG_LINK_UP, 0, len);
  else
    nsm_server_send_message (nse, ifp->vr->id, ifp->vrf->id,
                             NSM_MSG_LINK_DOWN, 0, len);

  return 0;
}

/* Address Add/Delete are done outside of context.  Hence VR-ID = 0. */
/* Send interface address information to client.  */
s_int32_t
nsm_server_send_interface_address (struct nsm_server_entry *nse,
                                   struct connected *ifc, int add)
{
  struct nsm_msg_address msg;
  struct prefix *p;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;
  s_int32_t len;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_address));

  /* Check requested service.  */
  if (! nsm_service_check (nse, NSM_SERVICE_INTERFACE))
    return 0;

  /* Check interface binding.  */
  if (! nsm_server_if_bind_check (nse, ifc->ifp))
    return 0;

#if 0
  /* Check interface membership. */
  if (! NSM_INTERFACE_CHECK_MEMBERSHIP (ifc->ifp, nse->service.protocol_id))
    return 0;
#endif

  msg.ifindex = ifc->ifp->ifindex;
  msg.flags = ifc->flags;

  /* Prefix information. */
  p = ifc->address;
  msg.prefixlen = p->prefixlen;
  if (p->family == AF_INET)
    {
      msg.afi = AFI_IP;
      msg.u.ipv4.src = p->u.prefix4;

      p = ifc->destination;
      if (p)
        msg.u.ipv4.dst = p->u.prefix4;
      else
        pal_mem_set (&msg.u.ipv4.dst, 0, sizeof msg.u.ipv4.dst);
    }
#ifdef HAVE_IPV6
  else if (NSM_CAP_HAVE_IPV6 && p->family == AF_INET6)
    {
      msg.afi = AFI_IP6;
      msg.u.ipv6.src = p->u.prefix6;

      p = ifc->destination;
      if (p)
        msg.u.ipv6.dst = p->u.prefix6;
      else
        pal_mem_set (&msg.u.ipv6.dst, 0, sizeof msg.u.ipv6.dst);
    }
#endif /* HAVE_IPV6 */

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_address (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  /* Send it to client.  */
  if (add)
    nsm_server_send_message (nse, vr_id, vrf_id,
                             NSM_MSG_ADDR_ADD, 0, len);
  else
    nsm_server_send_message (nse, vr_id, vrf_id,
                             NSM_MSG_ADDR_DELETE, 0, len);

  return 0;
}

/* Send IPv4 routes for redistribution.  */
s_int32_t
nsm_server_send_route_ipv4 (u_int32_t vr_id, vrf_id_t vrf_id,
                            struct nsm_server_entry *nse, u_int32_t msg_id,
                            struct nsm_msg_route_ipv4 *msg)
{
  s_int32_t nbytes;

  /* Debug */
  if (IS_NSM_DEBUG_SEND)
    {
      zlog_info (nse->ns->zg, "NSM SEND IPv4 route");
      nsm_route_ipv4_dump (nse->ns->zg, msg);
    }

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  nbytes = nsm_encode_route_ipv4 (&nse->send.pnt, &nse->send.size, msg);
  if (nbytes < 0)
    return nbytes;

  nsm_server_send_message (nse, vr_id, vrf_id,
                           NSM_MSG_ROUTE_IPV4, msg_id, nbytes);

  return 0;
}

#ifdef HAVE_IPV6
/* Send IPv6 routes for redistribution.  */
s_int32_t
nsm_server_send_route_ipv6 (u_int32_t vr_id, vrf_id_t vrf_id,
                            struct nsm_server_entry *nse, u_int32_t msg_id,
                            struct nsm_msg_route_ipv6 *msg)
{
  s_int32_t nbytes;

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  nbytes = nsm_encode_route_ipv6 (&nse->send.pnt, &nse->send.size, msg);
  if (nbytes < 0)
    return nbytes;

  nsm_server_send_message (nse, vr_id, vrf_id,
                           NSM_MSG_ROUTE_IPV6, msg_id, nbytes);

  return 0;
}
#endif /* HAVE_IPV6 */

#define IS_VIRTUALIZED(PM)        (MODBMAP_ISSET (PM_VR, PM))

s_int32_t
nsm_server_send_vr_add (struct nsm_server_entry *nse, struct apn_vr *vr)
{
  struct nsm_msg_vr msg;
  s_int32_t nbytes;
  s_int32_t vr_id = 0;
  s_int32_t vrf_id = 0;

  if (!IS_VIRTUALIZED (nse->service.protocol_id))
    return 0;

  msg.vr_id = vr->id;

  if (vr->name)
    {
      msg.len = pal_strlen (vr->name);
      msg.name = vr->name;
    }
  else
    msg.len = 0;

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  nbytes = nsm_encode_vr (&nse->send.pnt, &nse->send.size, &msg);
  if (nbytes < 0)
    return nbytes;

  nsm_server_send_message (nse, vr_id, vrf_id,
                           NSM_MSG_VR_ADD, 0, nbytes);

  return 0;
}

s_int32_t
nsm_server_send_vr_delete (struct nsm_server_entry *nse, struct apn_vr *vr)
{
  struct nsm_msg_vr msg;
  s_int32_t nbytes;
  s_int32_t vr_id = 0;
  s_int32_t vrf_id = 0;

  if (!IS_VIRTUALIZED (nse->service.protocol_id))
    return 0;

  msg.vr_id = vr->id;

  /* Omit name for Deletion. */
  msg.len = 0;
  msg.name = NULL;

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  nbytes = nsm_encode_vr (&nse->send.pnt, &nse->send.size, &msg);
  if (nbytes < 0)
    return nbytes;

  nsm_server_send_message (nse, vr_id, vrf_id,
                           NSM_MSG_VR_DELETE, 0, nbytes);

  return 0;
}

/* Send VRF information to client.  */
s_int32_t
nsm_server_send_vrf_add (struct nsm_server_entry *nse, struct apn_vrf *iv)
{
  struct nsm_msg_vrf msg;
  s_int32_t nbytes;
  vrf_id_t vrf_id = 0;

  msg.cindex = 0;

  /* Set VRF and FIB ID.  */
  msg.vrf_id = iv->id;
  msg.fib_id = iv->fib_id;

  /* Set VRF name.  */
  if (iv->name)
    {
      NSM_SET_CTYPE (msg.cindex, NSM_VRF_CTYPE_NAME);
      msg.len = pal_strlen (iv->name);
      msg.name = iv->name;
    }

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  nbytes = nsm_encode_vrf (&nse->send.pnt, &nse->send.size, &msg);
  if (nbytes < 0)
    return nbytes;

  nsm_server_send_message (nse, iv->vr->id, vrf_id,
                           NSM_MSG_VRF_ADD, 0, nbytes);

  return 0;
}

s_int32_t
nsm_server_send_vrf_delete (struct nsm_server_entry *nse, struct apn_vrf *iv)
{
  struct nsm_msg_vrf msg;
  s_int32_t nbytes;
  vrf_id_t vrf_id = 0;

  msg.cindex = 0;

  /* Set VRF and FIB ID.  */
  msg.vrf_id = iv->id;
  msg.fib_id = iv->fib_id;

  /* Set VRF name.  */
  if (iv->name)
    {
      NSM_SET_CTYPE (msg.cindex, NSM_VRF_CTYPE_NAME);
      msg.len = pal_strlen (iv->name);
      msg.name = iv->name;
    }

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  nbytes = nsm_encode_vrf (&nse->send.pnt, &nse->send.size, &msg);
  if (nbytes < 0)
    return nbytes;

  nsm_server_send_message (nse, iv->vr->id, vrf_id,
                           NSM_MSG_VRF_DELETE, 0, nbytes);

  return 0;
}

s_int32_t
nsm_server_send_vrf_add_all (struct nsm_master *nm,
                             struct nsm_server_entry *nse)
{
  struct nsm_vrf *nvrf;
  struct apn_vrf *iv;
  s_int32_t i;

  /* Send all VRF. */
  for (i = 0; i < vector_max (nm->vr->vrf_vec); i++)
    if ((iv = vector_slot (nm->vr->vrf_vec, i)))
      /* Default VRF Add message is sent to all the PMs.  */
      if (IS_APN_VRF_DEFAULT (iv)
          || nsm_service_check (nse, NSM_SERVICE_VRF))
        {
          nsm_server_send_vrf_add (nse, iv);

          nvrf = iv->proto;

#ifdef HAVE_MCAST_IPV4
          if (nvrf
              && nvrf->mcast
              && NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_IPV4_MRIB))
            {
              struct nsm_msg_ipv4_mrib_notification notif;

              pal_mem_set (&notif, 0, sizeof (notif));

              if (CHECK_FLAG (nvrf->mcast->config, MCAST_CONFIG_MRT))
                notif.type = NSM_MSG_IPV4_MRIB_NOTIFICATION_MCAST;
              else
                notif.type = NSM_MSG_IPV4_MRIB_NOTIFICATION_NO_MCAST;

              nsm_server_send_ipv4_mrib_notification (nm->vr->id,
                  iv->id, nse, &notif, NULL);
            }
#endif /* HAVE_MCAST_IPV4 */
#ifdef HAVE_MCAST_IPV6
          if (nvrf
              && nvrf->mcast6
              && NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_IPV6_MRIB))
            {
              struct nsm_msg_ipv6_mrib_notification notif;

              pal_mem_set (&notif, 0, sizeof (notif));

              if (CHECK_FLAG (nvrf->mcast6->config, MCAST6_CONFIG_MRT))
                notif.type = NSM_MSG_IPV6_MRIB_NOTIFICATION_MCAST;
              else
                notif.type = NSM_MSG_IPV6_MRIB_NOTIFICATION_NO_MCAST;

              nsm_server_send_ipv6_mrib_notification (nm->vr->id,
                  iv->id, nse, &notif, NULL);
            }
#endif /* HAVE_MCAST_IPV4 */
        }

  /* Interface Up/Down is done implicitly in library. XXX-VR */
  return 0;
}

#ifndef HAVE_IMI
s_int32_t
nsm_server_send_user_all (struct nsm_master *nm,
                          struct nsm_server_entry *nse)
{
  struct host *host = nm->vr->host;
  struct host_user *user;
  struct listnode *node;

  /* Send all user. */
  LIST_LOOP (host->users, user, node)
    nsm_server_send_user_update (nse, user, nm->vr->id);

  return 0;
}
#endif /* HAVE_IMI */

s_int32_t
nsm_server_send_router_id_all (struct nsm_master *nm,
                               struct nsm_server_entry *nse)
{
  struct apn_vrf *iv;
  s_int32_t i;

  for (i = 0; i < vector_max (nm->vr->vrf_vec); i++)
    if ((iv = vector_slot (nm->vr->vrf_vec, i)))
      if (IS_APN_VRF_DEFAULT (iv) || nsm_service_check (nse, NSM_SERVICE_VRF))
        if (iv->router_id.s_addr)
          nsm_server_send_router_id_update (nm->vr->id, iv->id,
                                            nse, iv->router_id);

  return 0;
}

#ifdef HAVE_MPLS
/* Send service message to NSM client.  */
s_int32_t
nsm_server_send_label_pool (u_int32_t vr_id,
                            struct nsm_server_entry *nse, u_int32_t msg_id,
                            struct nsm_msg_label_pool *msg, u_int16_t msg_type)
{
  s_int32_t len;
  vrf_id_t vrf_id = 0;

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_label_pool (&nse->send.pnt, &nse->send.size, msg);
  if (len < 0)
    return len;

  /* Send it to client.  */
  nsm_server_send_message (nse, vr_id, vrf_id, msg_type, msg_id, len);

  return 0;
}

#ifdef HAVE_PACKET
/* Send ILM lookup resut to NSM client. */
s_int32_t
nsm_server_send_ilm_lookup (u_int32_t vr_id,
                            struct nsm_server_entry *nse, u_int32_t msg_id,
                            struct nsm_msg_ilm_lookup *msg, u_int16_t msg_type)
{
  s_int32_t len;
  vrf_id_t vrf_id = 0;

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM message.  */
  len = nsm_encode_ilm_lookup (&nse->send.pnt, &nse->send.size, msg);
  if (len < 0)
    return len;

  /* Send it to client.  */
  nsm_server_send_message (nse, vr_id, vrf_id, msg_type, msg_id, len);

  return 0;
}
#endif /* HAVE_PACKET */

s_int32_t
nsm_server_send_ilm_gen_lookup (u_int32_t vr_id,
                                struct nsm_server_entry *nse, u_int32_t msg_id,
                                struct nsm_msg_ilm_gen_lookup *msg,
                                u_int16_t msg_type)
{
  s_int32_t len;
  vrf_id_t vrf_id = 0;

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM message.  */
  len = nsm_encode_ilm_gen_lookup (&nse->send.pnt, &nse->send.size, msg);
  if (len < 0)
    return len;

  /* Send it to client.  */
  nsm_server_send_message (nse, vr_id, vrf_id, msg_type, msg_id, len);

  return 0;
}


s_int32_t
nsm_server_send_igp_shortcut_lsp (struct nsm_server_entry *nse,
                                  struct ftn_entry *ftn,
                                  struct pal_in4_addr *t_endp,
                                  u_char action)
{
  struct nsm_msg_igp_shortcut_lsp msg;
  s_int32_t len;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  msg.action = action;
  msg.metric = ftn->lsp_bits.bits.lsp_metric;
  msg.tunnel_id = ftn->tun_id;
  msg.t_endp = *t_endp;

#if 0
  if (IS_NSM_DEBUG_SEND)
    nsm_igp_shortcut_lsp_dump (nse->ns->zg, &msg);
#endif

  len = nsm_encode_igp_shortcut_lsp (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  nsm_server_send_message (nse, vr_id, vrf_id,
                           NSM_MSG_MPLS_IGP_SHORTCUT_LSP, 0, len);

  return 0;
}

void
nsm_mpls_send_igp_shortcut_lsp (struct ftn_entry *ftn,
                                struct prefix *t_p, u_char action)
{
  s_int32_t i;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_IGP_SHORTCUT))
      nsm_server_send_igp_shortcut_lsp (nse, ftn, &t_p->u.prefix4, action);
}

#ifdef HAVE_RESTART
s_int32_t
nsm_gmpls_send_stale_bundle_msg (struct nsm_gen_msg_stale_info *msg)
{
  u_char buf[NSM_MESSAGE_MAX_LEN];
  u_int16_t size;
  u_char *pnt = NULL;
  s_int32_t nbytes;
  struct nsm_server_entry_buf *temp_buf = NULL;
  struct nsm_server_bulk_buf *temp_server_buf = NULL;
  struct nsm_server *ns = ng->server;
  struct nsm_msg_stale_info omsg;

  if (msg->in_label.type == gmpls_entry_type_ip)
    {
      prefix_copy (&omsg.fec_prefix, &msg->fec_prefix);
      omsg.iif_ix = msg->iif_ix;
      omsg.oif_ix = msg->oif_ix;
      omsg.ilm_ix = msg->ilm_ix;
      omsg.in_label = msg->in_label.u.pkt;
      omsg.out_label = msg->out_label.u.pkt;

      return nsm_mpls_send_stale_bundle_msg (&omsg);
    }

  temp_server_buf = &(ns->server_buf);
  temp_buf = &(temp_server_buf->bulk_buf);

  /* Set temporary pnt and size.  */
  pnt = buf;
  size = NSM_MESSAGE_MAX_LEN;

  /* Encode NSM stale bundle message. */
  nbytes = nsm_encode_gen_stale_bundle_msg (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  /* When bundle buffer is filled in flush it.  */
  if (nbytes >= temp_buf->len)
    {
      THREAD_TIMER_OFF (ns->server_buf.bulk_thread);
      thread_execute (nzg, nsm_gmpls_send_gen_stale_flush, &ns->server_buf, 0);
    }

  /* Set pnt and size.  */
  if (temp_buf->pnt)
    {
      pnt = temp_buf->pnt;
      size = temp_buf->len;
    }
  else
    {
      pnt = temp_buf->buf + NSM_MSG_HEADER_SIZE;
      size = temp_buf->len - NSM_MSG_HEADER_SIZE;
    }

  /* Copy the information.  */
  pal_mem_cpy (pnt, buf, nbytes);

  /* Update pointer and length.  */
  temp_buf->pnt = pnt + nbytes;
  temp_buf->len = size -= nbytes;

  /* Start timer.  */
  THREAD_TIMER_ON (nzg, ns->server_buf.bulk_thread,
                   nsm_gmpls_send_gen_stale_flush, &ns->server_buf,
                   NSM_SERVER_BUNDLE_TIME);

  return 0;
}

s_int32_t
nsm_mpls_send_stale_bundle_msg (struct nsm_msg_stale_info *msg)
{
  u_char buf[NSM_MESSAGE_MAX_LEN];
  u_int16_t size;
  u_char *pnt = NULL;
  s_int32_t nbytes;
  struct nsm_server_entry_buf *temp_buf = NULL;
  struct nsm_server_bulk_buf *temp_server_buf = NULL;
  struct nsm_server *ns = ng->server;

  temp_server_buf = &(ns->server_buf);
  temp_buf = &(temp_server_buf->bulk_buf);

  /* Set temporary pnt and size.  */
  pnt = buf;
  size = NSM_MESSAGE_MAX_LEN;

  /* Encode NSM stale bundle message. */
  nbytes = nsm_encode_stale_bundle_msg (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  /* When bundle buffer is filled in flush it.  */
  if (nbytes >= temp_buf->len)
    {
      THREAD_TIMER_OFF (ns->server_buf.bulk_thread);
      thread_execute (nzg, nsm_mpls_send_stale_flush, &ns->server_buf, 0);
    }

  /* Set pnt and size.  */
  if (temp_buf->pnt)
    {
      pnt = temp_buf->pnt;
      size = temp_buf->len;
    }
  else
    {
      pnt = temp_buf->buf + NSM_MSG_HEADER_SIZE;
      size = temp_buf->len - NSM_MSG_HEADER_SIZE;
    }

  /* Copy the information.  */
  pal_mem_cpy (pnt, buf, nbytes);

  /* Update pointer and length.  */
  temp_buf->pnt = pnt + nbytes;
  temp_buf->len = size -= nbytes;

  /* Start timer.  */
  THREAD_TIMER_ON (nzg, ns->server_buf.bulk_thread,
          nsm_mpls_send_stale_flush, &ns->server_buf,
          NSM_SERVER_BUNDLE_TIME);

  return 0;
}

s_int32_t
nsm_gmpls_send_gen_stale_flush (struct thread *t)
{
  s_int32_t nbytes;
  struct nsm_server_client *nsc = NULL;
  struct nsm_server_entry *nse = NULL;
  struct nsm_server_entry_buf *temp_buf = NULL;
  struct nsm_server_bulk_buf *temp_server_buf = NULL;
  s_int32_t i;

  temp_server_buf = THREAD_ARG (t);
  temp_buf = &(temp_server_buf->bulk_buf);
  temp_server_buf->bulk_thread = NULL;

  nbytes = temp_buf->pnt - temp_buf->buf - NSM_MSG_HEADER_SIZE;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)

  if (nse->service.protocol_id == APN_PROTO_LDP)
    {
      pal_mem_cpy (&nse->send, temp_buf, sizeof (struct nsm_server_entry_buf));

      nsm_server_send_message (nse, 0, 0, NSM_MSG_GEN_STALE_INFO, 0, nbytes);
    }

  temp_buf->pnt = NULL;
  temp_buf->len = NSM_MESSAGE_MAX_LEN;
  pal_mem_set (temp_buf->buf, 0, NSM_MESSAGE_MAX_LEN);
  return 0;
}

s_int32_t
nsm_mpls_send_stale_flush (struct thread *t)
{
  s_int32_t nbytes;
  struct nsm_server_client *nsc = NULL;
  struct nsm_server_entry *nse = NULL;
  struct nsm_server_entry_buf *temp_buf = NULL;
  struct nsm_server_bulk_buf *temp_server_buf = NULL;
  s_int32_t i;

  temp_server_buf = THREAD_ARG (t);
  temp_buf = &(temp_server_buf->bulk_buf);
  temp_server_buf->bulk_thread = NULL;

  nbytes = temp_buf->pnt - temp_buf->buf - NSM_MSG_HEADER_SIZE;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)

  if (nse->service.protocol_id == APN_PROTO_LDP)
    {
      pal_mem_cpy (&nse->send, temp_buf, sizeof (struct nsm_server_entry_buf));

      nsm_server_send_message (nse, 0, 0, NSM_MSG_STALE_INFO, 0, nbytes);
    }

  temp_buf->pnt = NULL;
  temp_buf->len = NSM_MESSAGE_MAX_LEN;
  pal_mem_set (temp_buf->buf, 0, NSM_MESSAGE_MAX_LEN);
  return 0;
}
#endif /* HAVE_RESTART */
#endif /* HAVE_MPLS */

#ifdef HAVE_LACPD

/* This function is used to sync the LAG configuration */

s_int32_t
nsm_server_send_lacp_config_sync (struct nsm_master *nm,
                                  struct nsm_server_entry *nse)
{
  struct interface *ifp;
  struct route_node *rn;
  struct nsm_if *zif = NULL;

  /* Send all interface information.  */
  for (rn = route_top (nm->vr->ifm.if_table); rn; rn = route_next (rn))
    if ((ifp = rn->info))
      /* Skip pseudo interface. */
#ifdef HAVE_SUPPRESS_UNMAPPED_IF
      if (CHECK_FLAG (ifp->status, NSM_INTERFACE_MAPPED))
#else /* HAVE_SUPPRESS_UNMAPPED_IF */
     if (CHECK_FLAG (ifp->status, NSM_INTERFACE_ACTIVE))
#endif /* HAVE_SUPPRESS_UNMAPPED_IF */
        {

          if (NSM_INTF_TYPE_AGGREGATED (ifp))
            {
               zif = ifp->info;

               if (zif->agg_config_type == AGG_CONFIG_LACP)
                 nsm_lacp_send_aggregator_config (ifp->ifindex,
                                                  ifp->lacp_agg_key,
                                                  zif->agg_mode, PAL_TRUE);
            }

        }

  return 0;
}

#endif /* HAVE_LACPD */

/* When client connection is established, NSM server send current
   interface information when the connection has request for Service
   Interface.  */
s_int32_t
nsm_server_send_interface_sync (struct nsm_master *nm,
                                struct nsm_server_entry *nse)
{
  struct interface *ifp;
  struct connected *ifc;
  struct route_node *rn;
#ifdef HAVE_LACPD
  struct nsm_if *zif = NULL;
#endif

  /* Send all interface information.  */
  for (rn = route_top (nm->vr->ifm.if_table); rn; rn = route_next (rn))
    if ((ifp = rn->info))
      /* Skip pseudo interface. */
#ifdef HAVE_SUPPRESS_UNMAPPED_IF
      if (CHECK_FLAG (ifp->status, NSM_INTERFACE_MAPPED))
#if 0
        && NSM_INTERFACE_CHECK_MEMBERSHIP(ifp, nse->service.protocol_id))
#endif
#else /* HAVE_SUPPRESS_UNMAPPED_IF */
     if (CHECK_FLAG (ifp->status, NSM_INTERFACE_ACTIVE))
#if 0
     && NSM_INTERFACE_CHECK_MEMBERSHIP(ifp, nse->service.protocol_id))
#endif
#endif /* HAVE_SUPPRESS_UNMAPPED_IF */
        {
          /* Send interface information.  */
          /* Dont send aggregated interface information to PM other than LACP*/
#ifdef HAVE_LACPD
         if (! NSM_INTF_TYPE_AGGREGATED (ifp))
#endif /* HAVE_LACPD */
           nsm_server_send_interface_add (nse, ifp);
#ifdef HAVE_LACPD
         else if (nse->service.protocol_id == APN_PROTO_LACP)
           nsm_server_send_interface_add (nse, ifp);
#endif /* HAVE_LACPD */

#ifdef HAVE_ONMD
         if (nse->service.protocol_id == APN_PROTO_ONM)
           nsm_server_send_interface_add (nse, ifp);
#endif /* HAVE_ONMD */

#ifdef HAVE_LACPD
          if (NSM_INTF_TYPE_AGGREGATED (ifp))
            {
               zif = ifp->info;
               if (zif->agg_config_type == AGG_CONFIG_LACP)
                   nsm_lacp_send_aggregator_config (ifp->ifindex, 
                                                    ifp->lacp_agg_key,
                                                    zif->agg_mode, PAL_TRUE);
            }
#endif

          /* Send interface address information.  */
          for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
            if (CHECK_FLAG (ifc->conf, NSM_IFC_REAL))
              nsm_server_send_interface_address (nse, ifc, NSM_IF_ADDRESS_ADD);
#ifdef HAVE_IPV6
          for (ifc = ifp->ifc_ipv6; ifc; ifc = ifc->next)
            if (CHECK_FLAG (ifc->conf, NSM_IFC_REAL))
              nsm_server_send_interface_address (nse, ifc, NSM_IF_ADDRESS_ADD);
#endif /* HAVE_IPV6 */
        }

  return 0;
}

/* Interface up information.  */
void
nsm_server_if_up_update (struct interface *ifp, cindex_t cindex)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
#ifdef HAVE_LACPD
  struct nsm_if *zif;
  struct nsm_master *nm = ifp->vr->proto;
  s_int32_t ret;
#endif /* HAVE_LACPD */
  u_int32_t i;

#ifdef HAVE_SUPPRESS_UNMAPPED_IF
  if (! CHECK_FLAG (ifp->status, NSM_INTERFACE_MAPPED))
    return;
#endif /* HAVE_SUPPRESS_UNMAPPED_IF */

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "UPDATE: Interface %s up", ifp->name);

#ifdef HAVE_LACPD
  {
    u_int16_t key = ifp->lacp_admin_key;
    nsm_lacp_if_admin_key_set (ifp);
    if (ifp->lacp_admin_key != key)
      {
        NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_LACP_KEY);

        /* Try to restore channel-group command if failed before */
        if ((zif = ifp->info))
          {
            if (zif->conf_key && zif->conf_chan_activate && zif->conf_agg_config_type)
            {
              ret = nsm_lacp_api_add_aggregator_member(nm, ifp,
                                         (u_int16_t)zif->conf_key,
                                         zif->conf_chan_activate, PAL_TRUE,
                                         zif->conf_agg_config_type);

              /* If Command gets restored successfully, make sure
                 that no restoration attempt is made again
              */
              if (ret == 0)
                {
                  zif->conf_key = 0;
                  zif->conf_chan_activate = PAL_FALSE;
                  zif->conf_agg_config_type = AGG_CONFIG_NONE;
                }
            }
          }
      }


  }
#endif /* HAVE_LACPD */

  /* Always send name. */
  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_NAME);

  if(ifp->bandwidth)
    NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_BANDWIDTH);

  if(ifp->duplex)
    NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_DUPLEX);

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_INTERFACE))
#if 0
        && NSM_INTERFACE_CHECK_MEMBERSHIP(ifp, nse->service.protocol_id))
#endif
      nsm_server_send_interface_state (nse, ifp, NSM_IF_UP , cindex);
}

/* Interface down information. */
void
nsm_server_if_down_update (struct interface *ifp, cindex_t cindex)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  u_int32_t i;

  nsc = NULL;
  nse = NULL;

#ifdef HAVE_SMI
  char ifname[SMI_INTERFACE_NAMSIZ + 1];
  snprintf(ifname, SMI_INTERFACE_NAMSIZ, ifp->name);
  /* FM recording */
  smi_record_fault (nzg, FM_GID_NSM,
                    NSM_SMI_ALARM_LOC,
                    __LINE__, __FILE__, ifname);
#endif /* HAVE_SMI */

#ifdef HAVE_SUPPRESS_UNMAPPED_IF
  if (! CHECK_FLAG (ifp->status, NSM_INTERFACE_MAPPED))
    return;
#endif /* HAVE_SUPPRESS_UNMAPPED_IF */

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "UPDATE: Interface %s down", ifp->name);

#ifdef HAVE_LACPD
  {
    u_int16_t key = ifp->lacp_admin_key;
    nsm_lacp_if_admin_key_set (ifp);
    if (ifp->lacp_admin_key != key)
      NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_LACP_KEY);
  }
#endif /* HAVE_LACPD */

  /* Always send name. */
  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_NAME);

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_INTERFACE))
#if 0
        && NSM_INTERFACE_CHECK_MEMBERSHIP(ifp, nse->service.protocol_id))
#endif
      nsm_server_send_interface_state (nse, ifp, NSM_IF_DOWN, cindex);

#ifdef HAVE_MPLS_OAM_ITUT
      if (itut_flag.flag  == NSM_MPLS_OAM_ENABLE)
           nsm_mpls_oam_itut_process_fdi_request(nse, ifp);
#endif /* HAVE_MPLS_OAM_ITUT*/

}

void
nsm_server_send_nsm_status (u_char nsm_status)
{
  struct nsm_server_client *nsc = NULL;
  struct nsm_server_entry *nse = NULL;
  struct nsm_msg_server_status msg;    
  int nbytes;                          
  u_int32_t i = 0;       

  pal_mem_set(&msg, 0, sizeof(struct nsm_msg_server_status)); 

  if (nsm_status == NSM_MSG_NSM_SERVER_SHUTDOWN)
    SET_FLAG (msg.nsm_status, NSM_MSG_NSM_SERVER_SHUTDOWN);

  /* Send NSM shut message to PM. */  
   NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
     {
       /* Set nse pointer and size. */
       nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
       nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

       /* Encode NSM status  message. */
       nbytes = nsm_encode_server_status_msg (&nse->send.pnt,
         &nse->send.size, &msg);
       if (nbytes < 0)
         return;

       nsm_server_send_message (nse, 0, 0, NSM_MSG_NSM_SERVER_STATUS, 0, 1);
      }
}
               

/* Interface add information.  */
void
nsm_server_if_add (struct interface *ifp)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  u_int32_t i;

#ifdef HAVE_SUPPRESS_UNMAPPED_IF
  if (! CHECK_FLAG (ifp->status, NSM_INTERFACE_MAPPED))
    return;

  if (ifp->ifindex == 0)
    return;
#endif /* HAVE_SUPPRESS_UNMAPPED_IF */

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "UPDATE: Interface %s update", ifp->name);

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_INTERFACE))
      {
        /* Send link add information.  */
        nsm_server_send_link_add (nse, ifp);

#if 0
        if (NSM_INTERFACE_CHECK_MEMBERSHIP (ifp, nse->service.protocol_id))
#endif
          {
            /* Send VR bind information.  */
            nsm_server_send_vr_bind (nse, ifp);

            /* Send VRF binding information.  */
            if (nsm_service_check (nse, NSM_SERVICE_VRF))
              nsm_server_send_vrf_bind (nse, ifp);
          }
      }
}

/* Interface delete information.  */
void
nsm_server_if_delete (struct interface *ifp)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  s_int32_t logical_deletion = 0;
  u_int32_t i;

#ifdef HAVE_SUPPRESS_UNMAPPED_IF
  if (! CHECK_FLAG (ifp->status, NSM_INTERFACE_MAPPED))
    return;

  if (ifp->ifindex == 0)
    return;
#endif /* HAVE_SUPPRESS_UNMAPPED_IF */

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "UPDATE: Interface %s delete", ifp->name);

  /* For L3 vlan interfaces wait for del done message from clients */
  if (if_is_vlan (ifp))
#if !defined HAVE_SWFWDR && (!defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING)
    logical_deletion = 1;
#else
    logical_deletion = 0;
#endif /* !defined HAVE_SWFWDR && (!defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING) */

#ifdef HAVE_LACPD


  if (NSM_INTF_TYPE_AGGREGATOR (ifp))
    {
        /* Already sent the delete message to protocols.
         * Do not send again
         */
      if (ifp->clean_pend_resp_list)
        {
          logical_deletion = 1;
        }
      else if (ifp->ifindex > 0)
        {
          /* For aggregator interfaces, if it has a valid ifindex,
           * then only clients will reply */
          logical_deletion = 0;
        }
      else
        logical_deletion = 1;
    }

#endif /* HAVE_LACPD */

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_INTERFACE))
      {
#if 0
        if (NSM_INTERFACE_CHECK_MEMBERSHIP (ifp, nse->service.protocol_id))
#endif
          {
            /* Send VRF unbind information.  */
            if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_VRF))
              if (NSM_IF_BIND_CHECK (ifp, VRF))
                nsm_server_send_vrf_unbind (nse, ifp);

            /* Send VR unbind information.  */
            nsm_server_send_vr_unbind (nse, ifp);
          }

        /* Send link delete information.  */
        nsm_server_send_link_delete (nse, ifp, logical_deletion);
      }

   if(!ifp->clean_pend_resp_list)
   {
      /* If no messages sent just delete the interface. */
      nsm_if_delete_update_done (ifp);
   }
}

/* Interface update information.  */
void
nsm_server_if_update (struct interface *ifp, cindex_t cindex)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  s_int32_t i;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_INTERFACE))
      nsm_server_send_link_update (nse, ifp, cindex);
}

void
nsm_server_if_bind_all (struct interface *ifp)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  s_int32_t i;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_INTERFACE))
#if 0
      if (NSM_INTERFACE_CHECK_MEMBERSHIP (ifp, nse->service.protocol_id))
#endif
        {
          /* Send VR bind information.  */
          nsm_server_send_vr_bind (nse, ifp);

          /* Send VRF binding information.  */
          if (nsm_service_check (nse, NSM_SERVICE_VRF))
            nsm_server_send_vrf_bind (nse, ifp);
        }
}

void
nsm_server_if_unbind_all (struct interface *ifp)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  s_int32_t i;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_INTERFACE))
#if 0
      if (NSM_INTERFACE_CHECK_MEMBERSHIP (ifp, nse->service.protocol_id))
#endif
        {
          /* Send VRF unbind information.  */
          if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_VRF))
            if (NSM_IF_BIND_CHECK (ifp, VRF))
              nsm_server_send_vrf_unbind (nse, ifp);

          /* Send VR unbind information.  */
          nsm_server_send_vr_unbind (nse, ifp);
        }
}

/* Interface address addition. */
void
nsm_server_if_address_add_update (struct interface *ifp, struct connected *ifc)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  u_int32_t i;

#ifdef HAVE_SUPPRESS_UNMAPPED_IF
  if (! CHECK_FLAG (ifp->status, NSM_INTERFACE_MAPPED))
    return;
#endif /* HAVE_SUPPRESS_UNMAPPED_IF */

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "UPDATE: Interface address add %O on %s",
               ifc->address, ifc->ifp->name);

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_INTERFACE))
#if 0
        && NSM_INTERFACE_CHECK_MEMBERSHIP(ifp, nse->service.protocol_id))
#endif
      nsm_server_send_interface_address (nse, ifc, NSM_IF_ADDRESS_ADD);
}

/* Interface address deletion. */
void
nsm_server_if_address_delete_update (struct interface *ifp,
                                     struct connected *ifc)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  u_int32_t i;

#ifdef HAVE_SUPPRESS_UNMAPPED_IF
  if (! CHECK_FLAG (ifp->status, NSM_INTERFACE_MAPPED))
    return;
#endif /* HAVE_SUPPRESS_UNMAPPED_IF */

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "UPDATE: Interface address delete %O on %s",
               ifc->address, ifc->ifp->name);

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_INTERFACE))
#if 0
        && NSM_INTERFACE_CHECK_MEMBERSHIP(ifp, nse->service.protocol_id))
#endif
      nsm_server_send_interface_address (nse, ifc, NSM_IF_ADDRESS_DELETE);
}

s_int32_t
nsm_server_send_router_id_update (u_int32_t vr_id, vrf_id_t vrf_id,
                                  struct nsm_server_entry *nse,
                                  struct pal_in4_addr router_id)
{
  s_int32_t len;
  struct nsm_master *nm = NULL;
  struct nsm_vrf *nv = NULL;
  struct nsm_msg_router_id msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_router_id));
  
  /* Check requested service. */
  if (!nsm_service_check (nse, NSM_SERVICE_ROUTER_ID))
    return 0;

  msg.cindex = 0;
  msg.router_id = router_id.s_addr;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return 0;

  nv = nsm_vrf_lookup_by_id (nm, vrf_id);
  if (nv == NULL)
    return 0;

  if (CHECK_FLAG (nv->config, NSM_VRF_CONFIG_ROUTER_ID))
    SET_FLAG (msg.config, NSM_ROUTER_ID_CONFIGURED);

  if (nv->router_id_type == NSM_ROUTER_ID_AUTOMATIC_LOOPBACK)
    SET_FLAG (msg.config, NSM_LOOPBACK_CONFIGURED);

  /* Set encode pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  len = nsm_encode_router_id (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  nsm_server_send_message (nse, vr_id, vrf_id,
                           NSM_MSG_ROUTER_ID_UPDATE, 0, len);

  return 0;
}

#ifdef HAVE_TE
s_int32_t
nsm_server_send_priority_bw (struct nsm_server_entry *nse,
                             struct interface *ifp)
{
  s_int32_t i;
  s_int32_t len;
  struct nsm_msg_link msg;

  /* TE service is needed.  */
  if (! nsm_service_check (nse, NSM_SERVICE_TE))
    return 0;

#ifdef HAVE_MPLS_VC
  /* Check VC interface binding.  */
  if (! nsm_service_check (nse, NSM_SERVICE_MPLS_VC)
      && (NSM_IF_BIND_CHECK (ifp, MPLS_VC) ||
                                  NSM_IF_BIND_CHECK (ifp, MPLS_VC_VLAN)))
    return 0;
#endif /* HAVE_MPLS_VC */

  /* Check VRF interface binding.  */
  if (! nsm_service_check (nse, NSM_SERVICE_VRF)
      && NSM_IF_BIND_CHECK (ifp, VRF))
    return 0;

  msg.ifindex = ifp->ifindex;
#ifdef HAVE_GMPLS
  msg.gifindex = ifp->gifindex;
#endif /* HAVE_GMPLS */
  msg.cindex = 0;
  NSM_SET_CTYPE (msg.cindex, NSM_LINK_CTYPE_PRIORITY_BW);
  for (i = 0; i < MAX_TE_CLASS; i++)
    msg.tecl_priority_bw[i] = ifp->tecl_priority_bw[i];


  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM interface.  */
  len = nsm_encode_link (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  /* Send it to client.  */
  nsm_server_send_message (nse, ifp->vr->id, ifp->vrf->id,
                           NSM_MSG_INTF_PRIORITY_BW_UPDATE, 0, len);

  return 0;
}

/* When new client is connected, send all interface's priority
   bandwidth information.  */
s_int32_t
nsm_server_send_priority_bw_sync (struct nsm_master *nm,
                                  struct nsm_server_entry *nse)
{
  struct interface *ifp;
  struct route_node *rn;
  s_int32_t ret;

  /* Go through iflist and send up updates for those interfaces that
     have a non-zero max-resv-bw and are not loopback */
  for (rn = route_top (nm->vr->ifm.if_table); rn; rn = route_next (rn))
    if ((ifp = rn->info))
      if (! if_is_loopback (ifp))
        {
          ret = nsm_server_send_priority_bw (nse, ifp);
          if (ret < 0)
            {
              route_unlock_node (rn);
              return ret;
            }
        }

  return 0;
}

/* Available Bandwidth information per interface */
void
nsm_server_if_priority_bw_update (struct interface *ifp)
{
  u_int32_t i;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "UPDATE: Priority bandwidth on %s", ifp->name);

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_TE))
      nsm_server_send_priority_bw (nse, ifp);
}

s_int32_t
nsm_server_send_admin_group (struct nsm_master *nm,
                             struct nsm_server_entry *nse)
{
  struct admin_group *array;
  struct nsm_msg_admin_group msg;
  u_int32_t i;
  s_int32_t len;
  vrf_id_t vrf_id = 0;

  if (nm->nmpls == NULL)
    return 0;

  array = NSM_MPLS->admin_group_array;

  if (! nsm_service_check (nse, NSM_SERVICE_TE))
    return 0;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_admin_group));

  for (i = 0; i < ADMIN_GROUP_MAX; i++)
    {
      msg.array[i].val = array[i].val;
      pal_mem_cpy (msg.array[i].name, array[i].name, ADMIN_GROUP_NAMSIZ);
    }

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM interface.  */
  len = nsm_encode_admin_group (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  /* Send it to client.  */
  nsm_server_send_message (nse, nm->vr->id, vrf_id,
                           NSM_MSG_ADMIN_GROUP_UPDATE, 0, len);

  return 0;
}

/* Admin Group change information */
void
nsm_admin_group_update (struct nsm_master *nm)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  u_int32_t i;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "UPDATE: Admin Group");

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_TE))
      nsm_server_send_admin_group (nm, nse);
}

#endif /* HAVE_TE */

#ifdef HAVE_MPLS_VC
s_int32_t
nsm_server_send_mpls_l2_circuit_add (struct nsm_master *nm,
                                     struct nsm_server_entry *nse,
                                     struct nsm_mpls_circuit *vc,
                                     u_int16_t vc_type)
{
  struct nsm_msg_mpls_vc msg;
  vrf_id_t vrf_id = 0;
  s_int32_t len;

  if (! nsm_service_check (nse, NSM_SERVICE_MPLS_VC))
    return 0;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_mpls_vc));

  /* Set id. */
  msg.vc_id = vc->id;

  /* Set end-point address. */
  NSM_SET_CTYPE (msg.cindex, NSM_MPLS_VC_CTYPE_NEIGHBOR);
  if (vc->address.family == AF_INET)
    {
      msg.afi = AFI_IP;
      msg.nbr_addr_ipv4 = vc->address.u.prefix4;
    }
#ifdef HAVE_IPV6
  else if (NSM_CAP_HAVE_IPV6 && vc->address.family == AF_INET6)
    {
      msg.afi = AFI_IP6;
      msg.nbr_addr_ipv6 = vc->address.u.prefix6;
    }
  else
    return 0;
#endif /* HAVE_IPV6 */

  /* Set control word. */
  NSM_SET_CTYPE (msg.cindex, NSM_MPLS_VC_CTYPE_CONTROL_WORD);
  msg.c_word = vc->cw;

  /* Send the PW Status */
  NSM_SET_CTYPE (msg.cindex, NSM_MPLS_VC_CTYPE_PW_STATUS);
  msg.pw_status = vc->pw_status;

#ifdef HAVE_MS_PW
  /* Send the MSPW Role */
  NSM_SET_CTYPE (msg.cindex, NSM_MPLS_VC_CTYPE_MS_PW_ROLE);
  msg.ms_pw_role = vc->ms_pw_role;
#endif /* HAVE_MS_PW */

#ifdef HAVE_VCCV
  /* Send CC types */
  NSM_SET_CTYPE (msg.cindex, NSM_MPLS_VC_CTYPE_VCCV_CCTYPES);
  msg.cc_types = vc->cc_types;

  /* Send CV types */
  NSM_SET_CTYPE (msg.cindex, NSM_MPLS_VC_CTYPE_VCCV_CVTYPES);
  msg.cv_types = vc->cv_types;
#endif /* HAVE_VCCV */

  if (vc->vc_info && vc->vc_info->mif)
    {
      /* Send ifindex. */
      NSM_SET_CTYPE (msg.cindex, NSM_MPLS_VC_CTYPE_IF_INDEX);
      msg.ifindex = vc->vc_info->mif->ifp->ifindex;

      /* Send vc type. */
      NSM_ASSERT (MPLS_VC_IS_VALID (vc->vc_info->vc_type));
      NSM_SET_CTYPE (msg.cindex, NSM_MPLS_VC_CTYPE_VC_TYPE);
      msg.vc_type = vc_type;

      /* Send group id. */
      NSM_SET_CTYPE (msg.cindex, NSM_MPLS_VC_CTYPE_GROUP_ID);
      if (vc->group)
        msg.grp_id = vc->group->id;
      else
        msg.grp_id = vc->vc_info->mif->ifp->ifindex;

      /* Send ifmtu */
      if ( vc->vc_info->mif->ifp->mtu > 0 )
        {
          NSM_SET_CTYPE (msg.cindex, NSM_MPLS_VC_CTYPE_IF_MTU);
          msg.ifmtu = vc->vc_info->mif->ifp->mtu;
        }
      /* Send vlan-id. */
      if ( vc->vc_info->vlan_id > 0 )
        {
          NSM_SET_CTYPE (msg.cindex, NSM_MPLS_VC_CTYPE_VLAN_ID);
          msg.vlan_id = vc->vc_info->vlan_id;
        }
    }
#ifdef HAVE_VPLS
  else if (vc->vpls)
    {
      /* send vpls id */
      NSM_SET_CTYPE (msg.cindex, NSM_MPLS_VC_CTYPE_VPLS_ID);
      msg.vpls_id = vc->vpls->vpls_id;

      /* Send vc type. */
      NSM_SET_CTYPE (msg.cindex, NSM_MPLS_VC_CTYPE_VC_TYPE);
      msg.vc_type = vc_type;

      /* Send vlan-id. */
      if (vc->vc_info && vc->vc_info->vlan_id != 0)
        {
          NSM_SET_CTYPE (msg.cindex, NSM_MPLS_VC_CTYPE_VLAN_ID);
          msg.vlan_id = vc->vc_info->vlan_id;
        }

      /* Send ifmtu */
      if (vc->vpls->ifmtu > 0)
        {
          NSM_SET_CTYPE (msg.cindex, NSM_MPLS_VC_CTYPE_IF_MTU);
          msg.ifmtu = vc->vpls->ifmtu;
        }
    }
#endif /* HAVE_VPLS */

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_mpls_vc (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  nsm_server_send_message (nse, nm->vr->id, vrf_id,
                           NSM_MSG_MPLS_VC_ADD, 0, len);

  return 0;
}

s_int32_t
nsm_server_send_mpls_l2_circuit_delete (struct nsm_master *nm,
                                        struct nsm_server_entry *nse,
                                        struct nsm_mpls_circuit *vc)
{
  struct nsm_msg_mpls_vc msg;
  vrf_id_t vrf_id = 0;
  s_int32_t len;

  if (! nsm_service_check (nse, NSM_SERVICE_MPLS_VC))
    return 0;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_mpls_vc));

  /* Set id. */
  msg.vc_id = vc->id;

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_mpls_vc (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  nsm_server_send_message (nse, nm->vr->id, vrf_id,
                           NSM_MSG_MPLS_VC_DELETE, 0, len);

  return 0;
}

s_int32_t
nsm_server_send_mpls_pw_status_update (struct nsm_master *nm,
                                       struct nsm_server_entry *nse,
                                       struct nsm_mpls_circuit *vc)
{
  struct nsm_msg_pw_status msg;
  vrf_id_t vrf_id =0;
  s_int32_t len;

   if (! nsm_service_check (nse, NSM_SERVICE_MPLS_VC))
    return 0;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_pw_status));

  /* Set id. */
  msg.vc_id = vc->id;

   /* Set PW Status for state updates */
  msg.pw_status = vc->pw_status;

#ifdef HAVE_VCCV
  if (vc->vc_fib)
    msg.in_vc_label = vc->vc_fib->in_label;
#endif /* HAVE_VCCV */

  if (IS_NSM_DEBUG_SEND)
    {
      zlog_info (nse->ns->zg, "NSM SEND MPLS PW Status");
      nsm_mpls_pw_status_msg_dump (nse->ns->zg, &msg);
    }

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_pw_status (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  nsm_server_send_message (nse, nm->vr->id, vrf_id,
                           NSM_MSG_MPLS_PW_STATUS, 0, len);

  return 0;
}

void
nsm_mpls_l2_circuit_add_send (struct nsm_mpls_if *mif,
                              struct nsm_mpls_circuit *vc)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  s_int32_t i;

  if (!vc->vc_info)
    return;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "UPDATE: MPLS VC add");

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_MPLS_VC) &&
        vc->fec_type_vc != PW_OWNER_MANUAL)
      nsm_server_send_mpls_l2_circuit_add (mif->nm, nse, vc,
                                           vc->vc_info->vc_type);
}

void
nsm_mpls_l2_circuit_del_send (struct nsm_mpls_if *mif,
                              struct nsm_mpls_circuit *vc)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  s_int32_t i;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "UPDATE: MPLS VC delete");

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_MPLS_VC) &&
        vc->fec_type_vc != PW_OWNER_MANUAL)
      nsm_server_send_mpls_l2_circuit_delete (mif->nm, nse, vc);
}

#ifdef HAVE_VPLS
void
nsm_vpls_spoke_vc_send_pw_status (struct nsm_vpls *vpls,
                                  struct nsm_mpls_circuit *vc)
{
  struct nsm_server_client *nsc = NULL;
  struct nsm_server_entry *nse = NULL;
  int i;

  if (listcount (vpls->vpls_info_list) == 0)
    return;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "UPDATE: MPLS PW State update");

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_MPLS_VC))
      nsm_server_send_mpls_pw_status_update (vpls->nm, nse, vc);
}
#endif /* HAVE_VPLS */

void
nsm_mpls_l2_circuit_send_pw_status (struct nsm_mpls_if *mif,
                                    struct nsm_mpls_circuit *vc)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  s_int32_t i;

  if (! mif || !vc)
    return;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "UPDATE: MPLS PW State update");

 if (vc->fec_type_vc != PW_OWNER_MANUAL)
   {
     NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
       if (nsm_service_check (nse, NSM_SERVICE_MPLS_VC))
         nsm_server_send_mpls_pw_status_update (mif->nm, nse, vc);
   }
#ifdef HAVE_VCCV
 else
   {
     u_int8_t bfd_cv_type_in_use =  CV_TYPE_NONE;

     if (vc->vc_fib)
       bfd_cv_type_in_use = oam_util_get_bfd_cvtype_in_use(vc->cv_types,
                                                   vc->vc_fib->remote_cv_types);
     if ((bfd_cv_type_in_use == CV_TYPE_BFD_IPUDP_DET_SIG) ||
         (bfd_cv_type_in_use == CV_TYPE_BFD_ACH_DET_SIG))
       {
         NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
           if (nse->service.client_id == APN_PROTO_OAM)
             nsm_server_send_mpls_pw_status_update (mif->nm, nse, vc);
       }
   }
#endif /* HAVE_VCCV */
}

#if 0 /* Not used */
void
nsm_mpls_l2_circuit_send_psn_state (struct nsm_mpls_if *mif,
                                    struct nsm_mpls_circuit *vc)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  s_int32_t i;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "UPDATE: MPLS VC PSN State update");

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_MPLS_VC))
      nsm_server_send_mpls_pw_status_update (mif->nm, nse, vc);
}
#endif

void
nsm_mpls_l2_circuit_if_bind (struct interface *ifp, u_int16_t vlan_id,
                             u_int8_t fec_type_vc)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  struct connected *ifc;
  cindex_t cindex = 0;
  s_int32_t i;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "L2-Circuit Interface Add");

#ifdef HAVE_L2
  if (ifp->type == IF_TYPE_L2)
    {
      struct nsm_if *zif = (struct nsm_if *)(ifp->info);

      if (zif && zif->type == NSM_IF_TYPE_L2 && zif->bridge)
        {
          nsm_bridge_api_set_port_state_all (ifp->vr->proto,
                                             zif->bridge->name,
                                             ifp,
                                             NSM_BRIDGE_PORT_STATE_FORWARDING);
        }
    }
#endif /* HAVE_L2 */

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_INTERFACE))
#if 0
        && NSM_INTERFACE_CHECK_MEMBERSHIP(ifp, nse->service.protocol_id))
#endif
      {
        if (nsm_service_check (nse, NSM_SERVICE_MPLS_VC))
          {
            NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_BINDING);

            if (if_is_up (ifp))
              nsm_server_send_interface_state (nse, ifp, NSM_IF_UP, cindex);
            else
              nsm_server_send_interface_state (nse, ifp, NSM_IF_DOWN, cindex);
          }
        else
          {
            for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
              nsm_server_send_interface_address (nse, ifc,
                                                 NSM_IF_ADDRESS_DELETE);
#ifdef HAVE_IPV6
            for (ifc = ifp->ifc_ipv6; ifc; ifc = ifc->next)
              nsm_server_send_interface_address (nse, ifc,
                                                 NSM_IF_ADDRESS_DELETE);

#endif /* HAVE_IPV6 */
            if (nse->service.protocol_id !=  APN_PROTO_IMI)
              nsm_server_send_link_delete (nse, ifp, 1);
          }
      }
}

void
nsm_mpls_l2_circuit_if_unbind (struct nsm_mpls_if *mif, u_int16_t vlan_id,
                               u_int8_t fec_type_vc)
{
  s_int32_t i;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  struct interface *ifp = mif->ifp;
  struct connected *ifc;
  cindex_t cindex = 0;

  if (!ifp)
    return;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "VPLS Interface Delete");

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_INTERFACE))
#if 0
        && NSM_INTERFACE_CHECK_MEMBERSHIP(ifp, nse->service.protocol_id))
#endif
      {
        if (nsm_service_check (nse, NSM_SERVICE_MPLS_VC))
          {
            NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_BINDING);
            if (if_is_up (ifp))
              nsm_server_send_interface_state (nse, ifp, NSM_IF_UP, cindex);
            else
              nsm_server_send_interface_state (nse, ifp, NSM_IF_DOWN, cindex);
          }
        else
          {
            /* Send interface information.  */
            if (NSM_IF_BIND_CHECK (ifp, MPLS_VC) ||
                NSM_IF_BIND_CHECK (ifp, MPLS_VC_VLAN) ||
                NSM_IF_BIND_CHECK (ifp, VPLS) ||
                NSM_IF_BIND_CHECK (ifp, VPLS_VLAN) ||
                nse->service.protocol_id ==  APN_PROTO_IMI)
              continue;

            nsm_server_send_interface_add (nse, ifp);

            for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
              nsm_server_send_interface_address (nse, ifc,
                                                 NSM_IF_ADDRESS_ADD);
#ifdef HAVE_IPV6
            for (ifc = ifp->ifc_ipv6; ifc; ifc = ifc->next)
              nsm_server_send_interface_address (nse, ifc,
                                                 NSM_IF_ADDRESS_ADD);
#endif /* HAVE_IPV6 */
          }
      }
}

void
nsm_mpls_l2_circuit_sync (struct nsm_master *nm)
{
  struct nsm_mpls_if *mif;
  struct listnode *ln, *node;
  struct nsm_mpls_vc_info *vc_info;

  if ((NSM_MPLS != NULL) && (NSM_MPLS->iflist != NULL))
    {
      LIST_LOOP (NSM_MPLS->iflist, mif, ln)
	if ((mif->ifp != NULL) && (! if_is_loopback (mif->ifp)))
          LIST_LOOP (mif->vc_info_list, vc_info, node)
            {
              if ((! CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_MPLS_VC)) &&
                  (! CHECK_FLAG (vc_info->if_bind_type,
                                 NSM_IF_BIND_MPLS_VC_VLAN)))
                continue;

              if (vc_info->u.vc)
                {
#ifdef HAVE_MS_PW
                  if (vc_info->u.vc->ms_pw == NULL)
#endif /* HAVE_MS_PW */
                    {
                      nsm_mpls_l2_circuit_if_bind (mif->ifp, vc_info->vlan_id,
                                                   vc_info->u.vc->fec_type_vc);
                      nsm_mpls_l2_circuit_add_send (mif, vc_info->u.vc);
                    }
                }
            }
    }
#ifdef HAVE_MS_PW
  if (NSM_MPLS && (NSM_MS_PW_HASH != NULL))
    {
      hash_iterate (NSM_MS_PW_HASH,
                    ((void (*) (struct hash_backet *, void *))\
                     nsm_ms_pw_sync),
                    nm);
    }
#endif /* HAVE_MS_PW */
}
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VPLS
s_int32_t
nsm_server_send_vpls_mac_withdraw (struct nsm_master *nm,
                                   struct nsm_server_entry *nse,
                                   u_int32_t vpls_id,
                                   u_char *mac_addrs,
                                   u_int16_t num)
{
  struct nsm_msg_vpls_mac_withdraw msg;
  vrf_id_t vrf_id = 0;
  s_int32_t len;

  /* To be sent only to LDP. */
  if (! nsm_service_check (nse, NSM_SERVICE_VPLS))
    return 0;

  /* Sanity check. */
  if ((num != 0 && mac_addrs == NULL) || (num == 0 && mac_addrs != NULL))
    return NSM_FAILURE;

  /* Fill message. */
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vpls_mac_withdraw));
  msg.vpls_id = vpls_id;
  msg.num = num;
  msg.mac_addrs = mac_addrs;

  if (IS_NSM_DEBUG_SEND)
    nsm_vpls_mac_withdraw_dump (NSM_ZG, &msg);

  /* set encode pointer and size */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode VPLS MAC Address Withdraw message. */
  len = nsm_encode_vpls_mac_withdraw (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  nsm_server_send_message (nse, nm->vr->id, vrf_id,
                           NSM_MSG_VPLS_MAC_WITHDRAW, 0, len);

  return 0;
}

s_int32_t
nsm_server_send_vpls_add (struct nsm_server_entry *nse,
                          struct nsm_vpls *vpls, cindex_t cindex)
{
  s_int32_t len;
  struct nsm_msg_vpls_add msg;
  bool_t send_all = NSM_FALSE;
  vrf_id_t vrf_id = 0;

  if (! nsm_service_check (nse, NSM_SERVICE_VPLS))
    return 0;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vpls_add));

  /* set id */
  msg.vpls_id = vpls->vpls_id;

  send_all = (~cindex == 0) ? NSM_TRUE : NSM_FALSE;

  /* set group id */
  if (send_all || NSM_CHECK_CTYPE (cindex, NSM_VPLS_CTYPE_GROUP_ID))
    {
      msg.grp_id = vpls->grp_id;
      NSM_SET_CTYPE (msg.cindex, NSM_VPLS_CTYPE_GROUP_ID);
    }

  /* set interface mtu */
  if (send_all || NSM_CHECK_CTYPE (cindex, NSM_VPLS_CTYPE_IF_MTU))
    {
      msg.ifmtu = vpls->ifmtu;
      NSM_SET_CTYPE (msg.cindex, NSM_VPLS_CTYPE_IF_MTU);
    }

  /* set vpls type */
  if (send_all || NSM_CHECK_CTYPE (cindex, NSM_VPLS_CTYPE_VPLS_TYPE))
    {
      msg.vpls_type = vpls->vpls_type;
      NSM_SET_CTYPE (msg.cindex, NSM_VPLS_CTYPE_VPLS_TYPE);
    }

  /* set control word */
  if (send_all || NSM_CHECK_CTYPE (cindex, NSM_VPLS_CTYPE_CONTROL_WORD))
    {
      msg.c_word = vpls->c_word;
      NSM_SET_CTYPE (msg.cindex, NSM_VPLS_CTYPE_CONTROL_WORD);
    }

  /* set mesh peers */
  if ((send_all || NSM_CHECK_CTYPE (cindex, NSM_VPLS_CTYPE_PEERS_ADDED))
      && vpls->mp_count > 0)
    {
      s_int32_t ctr;
      struct route_node *rn;
      struct nsm_vpls_peer *peer;

      msg.peers_added = XCALLOC (MTYPE_TMP,
                                 sizeof (struct addr_in) * vpls->mp_count);
      if (! msg.peers_added)
        return NSM_ERR_MEM_ALLOC_FAILURE;

      for (rn = route_top (vpls->mp_table), ctr = 0;
           rn != NULL && ctr < vpls->mp_count;
           rn = route_next (rn))
        {
          peer = rn->info;
          if (! peer)
            continue;

          pal_mem_cpy (&msg.peers_added[ctr], &peer->peer_addr,
                       sizeof (struct addr_in));
          ctr++;
        }

      msg.add_count = vpls->mp_count;
      NSM_SET_CTYPE (msg.cindex, NSM_VPLS_CTYPE_PEERS_ADDED);
    }

  /* set encode pointer and size */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* encode VPLS add */
  len = nsm_encode_vpls_add (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    {
      if (msg.peers_added)
        XFREE (MTYPE_TMP, msg.peers_added);
      return len;
    }

  nsm_server_send_message (nse, vpls->nm->vr->id,
                           vrf_id, NSM_MSG_VPLS_ADD, 0, len);

  if (msg.peers_added)
    XFREE (MTYPE_TMP, msg.peers_added);

  return 0;
}


s_int32_t
nsm_server_send_vpls_delete (struct nsm_server_entry *nse,
                             struct nsm_vpls *vpls)
{
  s_int32_t len;
  vrf_id_t vrf_id = 0;

  if (! nsm_service_check (nse, NSM_SERVICE_VPLS))
    return 0;

  /* set encode pointer and size */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* encode VPLS delete */
  len = nsm_encode_vpls_delete (&nse->send.pnt, &nse->send.size, vpls->vpls_id);
  if (len < 0)
    return len;

  nsm_server_send_message (nse, vpls->nm->vr->id, vrf_id,
                           NSM_MSG_VPLS_DELETE, 0, len);

  return 0;
}

s_int32_t
nsm_server_send_vpls_peer_add (struct nsm_server_entry *nse,
                               struct nsm_vpls *vpls, struct addr_in *addr)
{
  struct nsm_msg_vpls_add msg;
  vrf_id_t vrf_id = 0;
  s_int32_t len;

  if (! nsm_service_check (nse, NSM_SERVICE_VPLS))
    return 0;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vpls_add));

  /* set id */
  msg.vpls_id = vpls->vpls_id;

  /* set peer */
  msg.peers_added = XCALLOC (MTYPE_TMP, sizeof (struct addr_in));
  if (! msg.peers_added)
    return NSM_ERR_MEM_ALLOC_FAILURE;

  pal_mem_cpy (msg.peers_added, addr, sizeof (struct addr_in));
  msg.add_count = 1;
  NSM_SET_CTYPE (msg.cindex, NSM_VPLS_CTYPE_PEERS_ADDED);

  /* Set the Type for the VPLS peer - VC_TYPE_ETH_VLAN*/
  msg.vpls_type = vpls->vpls_type;
  NSM_SET_CTYPE (msg.cindex, NSM_VPLS_CTYPE_VPLS_TYPE);
  /* set encode pointer and size */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* encode VPLS add */
  len = nsm_encode_vpls_add (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    {
      if (msg.peers_added)
        XFREE (MTYPE_TMP, msg.peers_added);
      return len;
    }

  nsm_server_send_message (nse, vpls->nm->vr->id, vrf_id,
                           NSM_MSG_VPLS_ADD, 0, len);

  if (msg.peers_added)
    XFREE (MTYPE_TMP, msg.peers_added);

  return 0;
}

s_int32_t
nsm_server_send_vpls_peer_delete (struct nsm_server_entry *nse,
                                  struct nsm_vpls *vpls, struct addr_in *addr)
{
  struct nsm_msg_vpls_add msg;
  vrf_id_t vrf_id = 0;
  s_int32_t len;

  if (! nsm_service_check (nse, NSM_SERVICE_VPLS))
    return 0;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vpls_add));

  /* set id */
  msg.vpls_id = vpls->vpls_id;

  /* set peer */
  msg.peers_deleted = XCALLOC (MTYPE_TMP, sizeof (struct addr_in));
  if (! msg.peers_deleted)
    return NSM_ERR_MEM_ALLOC_FAILURE;

  pal_mem_cpy (msg.peers_deleted, addr, sizeof (struct addr_in));
  msg.del_count = 1;
  NSM_SET_CTYPE (msg.cindex, NSM_VPLS_CTYPE_PEERS_DELETED);

  /* set encode pointer and size */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* encode VPLS add */
  len = nsm_encode_vpls_add (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    {
      if (msg.peers_deleted)
        XFREE (MTYPE_TMP, msg.peers_deleted);
      return len;
    }

  nsm_server_send_message (nse, vpls->nm->vr->id,
                           vrf_id, NSM_MSG_VPLS_ADD, 0, len);

  if (msg.peers_deleted)
    XFREE (MTYPE_TMP, msg.peers_deleted);

  return 0;
}

s_int32_t
nsm_vpls_mac_withdraw_send (struct nsm_master *nm,
                            u_int32_t vpls_id,
                            u_char *mac_addrs,
                            u_int16_t num)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  s_int32_t i;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "VPLS MAC Address Withdraw");

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_VPLS))
      nsm_server_send_vpls_mac_withdraw (nm, nse, vpls_id, mac_addrs, num);

  return 0;
}

s_int32_t
nsm_vpls_add_send (struct nsm_vpls *vpls, cindex_t cindex)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  s_int32_t i;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "VPLS add/update");

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_VPLS))
      nsm_server_send_vpls_add (nse, vpls, cindex);

  return 0;
}

s_int32_t
nsm_vpls_delete_send (struct nsm_vpls *vpls)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  s_int32_t i;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "VPLS delete");

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_VPLS))
      nsm_server_send_vpls_delete (nse, vpls);

  return 0;
}

s_int32_t
nsm_vpls_peer_add_send (struct nsm_vpls *vpls, struct addr_in *addr)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  s_int32_t i;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "VPLS Peer add");

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_VPLS))
      nsm_server_send_vpls_peer_add (nse, vpls, addr);

  return 0;
}

s_int32_t
nsm_vpls_peer_delete_send (struct nsm_vpls *vpls, struct addr_in *addr)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  s_int32_t i;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "VPLS Peer delete");

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_VPLS))
      nsm_server_send_vpls_peer_delete (nse, vpls, addr);

  return 0;
}

s_int32_t
nsm_vpls_spoke_vc_add_send (struct nsm_mpls_circuit *vc, u_int16_t vc_type)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  s_int32_t i;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "VPLS VC Add");

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_VPLS) &&
       (vc->fec_type_vc != PW_OWNER_MANUAL))
      nsm_server_send_mpls_l2_circuit_add (vc->vpls->nm, nse, vc, vc_type);

  return 0;
}

s_int32_t
nsm_vpls_spoke_vc_delete_send (struct nsm_mpls_circuit *vc)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  s_int32_t i;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "VPLS VC Delete");

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_VPLS) &&
       (vc->fec_type_vc != PW_OWNER_MANUAL))
      nsm_server_send_mpls_l2_circuit_delete (vc->vpls->nm, nse, vc);

  return 0;
}

s_int32_t
nsm_vpls_if_add_send (struct interface *ifp, u_int16_t vlan_id)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  struct connected *ifc;
  cindex_t cindex = 0;
  s_int32_t i;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "VPLS Interface Add");

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_INTERFACE))
#if 0
        && NSM_INTERFACE_CHECK_MEMBERSHIP(ifp, nse->service.protocol_id))
#endif
      {
        if (nsm_service_check (nse, NSM_SERVICE_VPLS))
          {
            NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_BINDING);

            if (if_is_running (ifp))
              nsm_server_send_interface_state (nse, ifp, NSM_IF_UP, cindex);
            else
              nsm_server_send_interface_state (nse, ifp, NSM_IF_DOWN, cindex);
          }
        else
          {
            for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
              nsm_server_send_interface_address (nse, ifc,
                                                 NSM_IF_ADDRESS_DELETE);
#ifdef HAVE_IPV6
            for (ifc = ifp->ifc_ipv6; ifc; ifc = ifc->next)
              nsm_server_send_interface_address (nse, ifc,
                                                 NSM_IF_ADDRESS_DELETE);

#endif /* HAVE_IPV6 */
            if (nse->service.protocol_id !=  APN_PROTO_IMI)
              nsm_server_send_link_delete (nse, ifp, 1);
          }
      }

  return 0;
}

s_int32_t
nsm_vpls_if_delete_send (struct interface *ifp)
{
  s_int32_t i;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  struct connected *ifc;
  cindex_t cindex = 0;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "VPLS Interface Delete");

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_INTERFACE))
#if 0
        && NSM_INTERFACE_CHECK_MEMBERSHIP(ifp, nse->service.protocol_id))
#endif
      {
        if (nsm_service_check (nse, NSM_SERVICE_VPLS))
          {
            NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_BINDING);
            if (if_is_running (ifp))
              nsm_server_send_interface_state (nse, ifp, NSM_IF_UP, cindex);
            else
              nsm_server_send_interface_state (nse, ifp, NSM_IF_DOWN, cindex);
          }
        else
          {
            /* Send interface information.  */
            if (NSM_IF_BIND_CHECK (ifp, MPLS_VC) ||
                NSM_IF_BIND_CHECK (ifp, MPLS_VC_VLAN) ||
                NSM_IF_BIND_CHECK (ifp, VPLS) ||
                NSM_IF_BIND_CHECK (ifp, VPLS_VLAN) ||
                nse->service.protocol_id ==  APN_PROTO_IMI)
              continue;

            nsm_server_send_interface_add (nse, ifp);

            for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
              nsm_server_send_interface_address (nse, ifc,
                                                 NSM_IF_ADDRESS_ADD);
#ifdef HAVE_IPV6
            for (ifc = ifp->ifc_ipv6; ifc; ifc = ifc->next)
              nsm_server_send_interface_address (nse, ifc,
                                                 NSM_IF_ADDRESS_ADD);
#endif /* HAVE_IPV6 */
          }
      }

  return 0;
}

s_int32_t
nsm_vpls_vlan_if_delete_send (struct interface *ifp)
{
  s_int32_t i;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  struct interface ifp_tmp;
  cindex_t cindex = 0;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "VPLS Interface Delete");

  pal_mem_cpy (&ifp_tmp, ifp, sizeof (struct interface));

  UNSET_FLAG (ifp_tmp.bind, NSM_IF_BIND_VPLS_VLAN);

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_INTERFACE))
#if 0
        && NSM_INTERFACE_CHECK_MEMBERSHIP(ifp, nse->service.protocol_id))
#endif
      {
        if (nsm_service_check (nse, NSM_SERVICE_VPLS))
          {
            NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_BINDING);
            if (if_is_up (&ifp_tmp))
              nsm_server_send_interface_state (nse, &ifp_tmp, NSM_IF_UP,
                                               cindex);
            else
              nsm_server_send_interface_state (nse, &ifp_tmp, NSM_IF_DOWN,
                                               cindex);
          }
      }

  return 0;
}
s_int32_t
nsm_vpls_sync_send (struct nsm_master *nm,
                    struct nsm_server_entry *nse)
{
  struct nsm_vpls *vpls;
  struct ptree_node *pn;
  struct listnode *ln;
  struct nsm_vpls_spoke_vc *svc;

  if (! NSM_MPLS || ! NSM_MPLS->vpls_table)
    return NSM_ERR_INVALID_ARGS;

  for (pn = ptree_top (NSM_MPLS->vpls_table); pn; pn = ptree_next (pn))
    {
      if (! pn->info)
        continue;

      vpls = pn->info;

      if (vpls->state == NSM_VPLS_STATE_ACTIVE)
        {
          nsm_server_send_vpls_add (nse, vpls, NSM_VPLS_ATTR_FLAG_ALL);

          LIST_LOOP (vpls->svc_list, svc, ln)
            if (svc->vc)
              nsm_server_send_mpls_l2_circuit_add (nm, nse, svc->vc,
                                                   svc->vc_type);
        }
    }

  return NSM_SUCCESS;
}
#endif /* HAVE_VPLS */

#ifdef HAVE_GMPLS
s_int32_t
nsm_server_send_gmpls_if_update (struct nsm_server_entry *nse,
                                 struct interface *ifp)
{
#if 0 /* TBD old code need remove */
  s_int32_t len;
  struct nsm_msg_gmpls_if msg;

  if (! nsm_service_check (nse, NSM_SERVICE_GMPLS))
    return 0;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_gmpls_if));
  msg.cindex = 0;

  msg.ifindex = ifp->ifindex;

  NSM_SET_CTYPE (msg.cindex, NSM_GMPLS_LINK_IDENTIFIER);
  msg.link_ids.local = ifp->gmpls_if->link_ids.local;
  msg.link_ids.remote = ifp->gmpls_if->link_ids.remote;

  NSM_SET_CTYPE (msg.cindex, NSM_GMPLS_PROTECTION_TYPE);
  msg.protection = ifp->gmpls_if->protection;

  NSM_SET_CTYPE (msg.cindex, NSM_GMPLS_CAPABILITY_TYPE);
  msg.capability = ifp->gmpls_if->capability;

  NSM_SET_CTYPE (msg.cindex, NSM_GMPLS_ENCODING_TYPE);
  msg.encoding = ifp->gmpls_if->encoding;

  NSM_SET_CTYPE (msg.cindex, NSM_GMPLS_MIN_LSP_BW);
  msg.min_lsp_bw = ifp->gmpls_if->min_lsp_bw;

  NSM_SET_CTYPE (msg.cindex, NSM_GMPLS_SDH_INDICATION);
  msg.indication = ifp->gmpls_if->indication;

  NSM_SET_CTYPE (msg.cindex, NSM_GMPLS_SHARED_RISK_LINK_GROUP);
  msg.srlg = ifp->gmpls_if->srlg;

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM interface.  */
  len = nsm_encode_gmpls_if (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  /* Send it to client.  */
  nsm_server_send_message (nse, ifp->vr->id, ifp->vrf->id,
                           NSM_MSG_GMPLS_IF, 0, len);

#endif
  return 0;
}

/* GMPLS interface TE update. */
void
nsm_gmpls_if_update (struct interface *ifp)
{
#if 0 /* TBD old code need remove */
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  s_int32_t i;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "UPDATE: GMPLS interface TE data");

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_GMPLS))
      nsm_server_send_gmpls_if_update (nse, ifp);
#endif
}

s_int32_t
nsm_server_send_gmpls_if_sync (struct nsm_master *nm,
                               struct nsm_server_entry *nse)
{
#if 0 /* TBD old code need remove */
  struct interface *ifp;
  struct route_node *rn;
nse
  /* Go through iflist and send up updates for those interfaces that
     have a non-zero max-resv-bw and are not loopback */
  for (rn = route_top (nm->vr->ifm.if_table); rn; rn = route_next (rn))
    if ((ifp = rn->info))
      if (ifp->gmpls_if)
        nsm_gmpls_if_update (ifp);
#endif
  return 0;
}

s_int32_t
nsm_server_send_data_link_sub_sync (struct nsm_master *nm,
                                    struct nsm_server_entry *nse)
{
  struct gmpls_if *gmif;
  struct avl_node *node;
  struct datalink *dl;

  /* Get the GMPLS interface structure for the same */
  gmif = gmpls_if_get (nm->zg, nm->vr->id);
  if (gmif == NULL)
    return -1;

  /* Go through all datalinks and send an add */
  for (node = avl_top (&gmif->dltree); node; node = avl_next (node))
    {
      dl = (struct datalink *) node->info;
      if (dl != NULL)
        {
          nsm_server_send_data_link_sub (gmif->vr->id, gmif->vrf->id, nse, dl,
                                         NSM_DATALINK_CTYPE_ADD);
        }
    }

  return 0;
}

s_int32_t
nsm_server_send_data_link_sync (struct nsm_master *nm,
                                struct nsm_server_entry *nse)
{
  struct gmpls_if *gmif;
  struct avl_node *node;
  struct datalink *dl;

  /* Get the GMPLS interface structure for the same */
  gmif = gmpls_if_get (nm->zg, nm->vr->id);
  if (gmif == NULL)
    return -1;

  /* Go through all datalinks and send an add */
  for (node = avl_top (&gmif->dltree); node; node = avl_next (node))
    {
      dl = (struct datalink *) node->info;
      if (dl != NULL)
        {
          nsm_server_send_data_link (gmif->vr->id, gmif->vrf->id, nse, dl,
                                     NSM_DATALINK_CTYPE_ADD);
        }
    }

  return 0;
}

s_int32_t
nsm_server_send_data_link_mod (vrf_id_t vrid, struct lib_globals *zg,
                               struct datalink *dl, cindex_t cindex)
{
  struct nsm_master *nm;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  struct apn_vr *vr;
  struct gmpls_if *gmif;
  s_int32_t i;

  if (!dl || !zg || !dl->gmif)
    return -1;

  vr = dl->gmif->vr;

  if (vr == NULL || vr->proto == NULL)
    return -1;

  nm = vr->proto;

  /* Get the GMPLS interface structure for the same */
  gmif = gmpls_if_get (nm->zg, nm->vr->id);
  if (gmif == NULL)
    return -1;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Datalink update");

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    {
      if (nsm_service_check (nse, NSM_SERVICE_DATA_LINK))
        {
          nsm_server_send_data_link (gmif->vr->id, gmif->vrf->id, nse, dl,
                                     cindex);
        }
      else
       {
         /* Is only link property set - if so we dont need to pass to the
            Datalink sub-service*/
         if (cindex != NSM_DATALINK_CTYPE_PROPERTY)
           {
             if (nsm_service_check (nse, NSM_SERVICE_DATA_LINK_SUB))
               {
                 nsm_server_send_data_link_sub (gmif->vr->id, gmif->vrf->id,
                                                nse, dl, cindex);
               }
           }
       }
    }

  return 0;
}

s_int32_t
nsm_server_send_te_link_sync (struct nsm_master *nm,
                              struct nsm_server_entry *nse)
{
  struct gmpls_if *gmif;
  struct avl_node *node;
  struct telink *tl;

  /* Get the GMPLS interface structure for the same */
  gmif = gmpls_if_get (nm->zg, nm->vr->id);
  if (gmif == NULL)
    return -1;

  /* Go through all datalinks and send an add */
  for (node = avl_top (&gmif->teltree); node; node = avl_next (node))
    {
      tl = (struct telink *) node->info;
      if (tl != NULL)
        {
          nsm_server_send_te_link (gmif->vr->id, gmif->vrf->id, nse, tl,
                                   NSM_TELINK_CTYPE_ADD);
        }
    }

  return 0;
}

s_int32_t
nsm_server_send_te_link_mod (vrf_id_t vrid, struct lib_globals *zg,
                             struct telink *tl, cindex_t cindex)
{
  struct nsm_master *nm;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  struct apn_vr *vr;
  struct gmpls_if *gmif;
  s_int32_t i;

  if (!tl || !zg || !tl->gmif)
    return -1;

  vr = tl->gmif->vr;

  if (vr == NULL || vr->proto == NULL)
    return -1;

  nm = vr->proto;

  /* Get the GMPLS interface structure for the same */
  gmif = gmpls_if_get (nm->zg, nm->vr->id);
  if (gmif == NULL)
    return -1;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "TE link update");

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    {
      if (nsm_service_check (nse, NSM_SERVICE_TE_LINK))
        {
          nsm_server_send_te_link (gmif->vr->id, gmif->vrf->id, nse,
                                   tl, cindex);
        }
    }

  return 0;
}

/* Send service message to NSM client.  */
s_int32_t
nsm_server_send_generic_label_pool (u_int32_t vr_id,
                                    struct nsm_server_entry *nse, u_int32_t msg_id,
                                    struct nsm_msg_generic_label_pool *msg,
                                    u_int16_t msg_type)
{
  s_int32_t len;
  vrf_id_t vrf_id = 0;

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_generic_label_pool (&nse->send.pnt, &nse->send.size, msg);
  if (len < 0)
    return len;

  /* Send it to client.  */
  nsm_server_send_message (nse, vr_id, vrf_id, msg_type, msg_id, len);

  return 0;
}

#endif /* HAVE_GMPLS */

void
nsm_server_send_vr_sync_done (struct nsm_server_entry *nse)
{
  if (nse == NULL)
    return;

  nsm_server_send_message (nse, 0, 0, NSM_MSG_VR_SYNC_DONE, 0, 1);
  return;
}
s_int32_t
nsm_server_send_sync_vr (struct nsm_master *nm,
                         struct nsm_server_entry *nse)
{
  s_int32_t ret;

  /* Send VR Add. */
  if (nm->vr->id != 0)
    if (MODBMAP_ISSET (nm->module_bits, nse->service.protocol_id))
      nsm_server_send_vr_add (nse, nm->vr);

#ifndef HAVE_IMI
  /* Send user information to client which request the service. */
  if ((ret = nsm_server_send_user_all (nm, nse)) < 0)
    return ret;
#endif /* HAVE_IMI */

  /* Send current VRF information to client which request the service. */
  if ((ret = nsm_server_send_vrf_add_all (nm, nse)) < 0)
    return ret;

  /* Send current interface information to client which reqest the service. */
  if (nsm_service_check (nse, NSM_SERVICE_INTERFACE))
    if ((ret = nsm_server_send_interface_sync (nm, nse)) < 0)
      return ret;

  /* Send Router-ID. */
  if (nsm_service_check (nse, NSM_SERVICE_ROUTER_ID))
    if ((ret = nsm_server_send_router_id_all (nm, nse)) < 0)
      return ret;

#ifdef HAVE_TE
  /* Send interface priority bandwidth to client.  */
  if (nsm_service_check (nse, NSM_SERVICE_TE))
    if ((ret = nsm_server_send_priority_bw_sync (nm, nse)) < 0)
      return ret;

  /* Admin group update.  */
  if (nsm_service_check (nse, NSM_SERVICE_TE))
    if ((ret = nsm_server_send_admin_group (nm, nse)) < 0)
      return ret;
#endif /* HAVE_TE */

#ifdef HAVE_MPLS_VC
  /* VC synchronization.  */
  if (nsm_service_check (nse, NSM_SERVICE_MPLS_VC))
    nsm_mpls_l2_circuit_sync (nm);
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VPLS
  /* VPLS synchronization */
  if (nsm_service_check (nse, NSM_SERVICE_VPLS))
    if ((ret = nsm_vpls_sync_send (nm, nse)) < 0)
      return ret;
#endif /* HAVE_VPLS */

#ifdef HAVE_GMPLS
  /* GMPLS interface TE update. */
#if 0 /* TBD old code remove */
  if (nsm_service_check (nse, NSM_SERVICE_GMPLS))
    if ((ret = nsm_server_send_gmpls_if_sync (nm, nse)) < 0)
      return ret;
#endif /* if 0 */

  if (nsm_service_check (nse, NSM_SERVICE_CONTROL_ADJ))
    if ((ret = nsm_server_send_gmpls_control_adj_sync (nm, nse)) < 0)
      return ret;

  if (nsm_service_check (nse, NSM_SERVICE_CONTROL_CHANNEL))
    if ((ret = nsm_server_send_gmpls_control_channel_sync (nm, nse)) < 0)
      return ret;

  if (nsm_service_check (nse, NSM_SERVICE_TE_LINK))
    if ((ret = nsm_server_send_te_link_sync (nm, nse)) < 0)
      return ret;

  if (nsm_service_check (nse, NSM_SERVICE_DATA_LINK_SUB))
    if ((ret = nsm_server_send_data_link_sub_sync (nm, nse)) < 0)
      return ret;

  if (nsm_service_check (nse, NSM_SERVICE_DATA_LINK))
    if ((ret = nsm_server_send_data_link_sync (nm, nse)) < 0)
      return ret;

#endif /* HAVE_GMPLS */

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
  /* DiffServ Info update. */
  if (nsm_service_check (nse, NSM_SERVICE_DIFFSERV))
    if (NSM_MPLS != NULL)
      {
        s_int32_t i;
        for (i = 0; i < DIFFSERV_MAX_SUPPORTED_DSCP; i++)
          if (NSM_MPLS->supported_dscp[i] == DIFFSERV_DSCP_SUPPORTED)
            nsm_mpls_supported_dscp_update (nm, i, DIFFSERV_DSCP_SUPPORTED);

        for (i = 0; i < DIFFSERV_MAX_DSCP_EXP_MAPPINGS; i++)
          if (NSM_MPLS->dscp_exp_map[i] != DIFFSERV_INVALID_DSCP)
            nsm_mpls_dscp_exp_map_update (nm, i, NSM_MPLS->dscp_exp_map[i]);
      }
#endif /* HAVE_DIFFSERV */

#ifdef HAVE_DSTE
  /* DSTE Info update. */
  if (nsm_service_check (nse, NSM_SERVICE_DSTE))
    if (NSM_MPLS != NULL)
      {
        s_int32_t i;
        for (i = 0; i < MAX_TE_CLASS; i++)
          if (NSM_MPLS->te_class[i].ct_num != CT_NUM_INVALID )
            nsm_dste_te_class_update (nm, i);
      }
#endif /* HAVE_DSTE */
#endif /* (!defined (HAVE_GMPLS) || defined (HAVE_PACKET)) */

#ifdef HAVE_L2
#ifdef HAVE_IMI
  if (((nse->service.client_id == APN_PROTO_ONM)
       || (nse->service.client_id == APN_PROTO_MSTP))
      && (!FLAG_ISSET (nm->vr->host->flags, HOST_CONFIG_READ_DONE)))
    {
      return 1;
    }
#endif /* HAVE_IMI */
  nsm_bridge_sync (nm, nse);

#ifdef HAVE_VLAN
  nsm_bridge_vlan_sync(nm, nse);
#endif /* HAVE_VLAN */

#ifdef HAVE_G8031
   nsm_g8031_sync (nm, nse);
#endif /* HAVE_G8031 */

#ifdef HAVE_G8032
  nsm_g8032_sync (nm, nse);
#endif /* HAVE_G8032 */

#endif /* HAVE_L2 */

#if defined HAVE_I_BEB || defined HAVE_B_BEB
  nsm_pbb_sync (nm, nse);
#endif /* HAVE_I_BEB || HAVE_B_BEB */

#ifdef HAVE_ELMID
  nsm_elmi_sync (nm, nse);
#endif /* HAVE_ELMID */

  nsm_server_send_vr_sync_done (nse);
  return 0;
}


/* Receive service request.  */
s_int32_t
nsm_server_recv_service (struct nsm_msg_header *header,
                         void *arg, void *message)
{
  struct nsm_server_client *nsc = NULL;
  struct nsm_server_entry *nse = arg;
  struct nsm_server *ns = nse->ns;
  struct nsm_msg_service *service = message;
  struct nsm_restart *restart;
  struct nsm_master *nm = NULL;
  struct apn_vr *vr;
  u_int32_t index;
  s_int32_t ret;
  s_int32_t i;

  /* Dump received messsage. */
  nse->service = *service;

  if (IS_NSM_DEBUG_RECV)
    nsm_service_dump (ns->zg, &nse->service);

  /* Get VR context. */
  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* When received client id is zero, we assgin new client id for it.  */
  if (nse->service.client_id)
    {
      index = service->client_id;
      if (index < vector_size (ns->client))
        nsc = vector_slot (ns->client, index);
    }

  if (! nsc)
    {
      /* Get new client id.  This is little bit tricky in vector index
         starts from zero, but client_id should be non zero.  So we
         add one to vector index for client id.  */
      nsc = XCALLOC (MTYPE_NSM_SERVER_CLIENT,
          sizeof (struct nsm_server_client));

      /* XXX: Client_id is local to a system and therefore cannot be
       * checkpointed. But it is used for Graceful Restart mechanism to
       * mark the routes STALE based on client id.
       * Therefore, for HA the client id will be the protocol id. This will
       * be ensured by assigning the client_id as the protocol_id at time
       * of NSM client connect (in nsm_server_recv_service() ).
       */
      index = vector_set_index (ns->client, nse->service.protocol_id, nsc);

      nsc->client_id = index;
      nse->service.client_id = nsc->client_id;
    }

  /* Add NSM client entry to NSM client list.  */
  nse->prev = nsc->tail;
  if (nsc->head == NULL)
    nsc->head = nse;
  else
    nsc->tail->next = nse;
  nsc->tail = nse;

  /* Remember which NSM sever client nse belongs to.  */
  nse->nsc = nsc;

  /* Send back server side service bits to client.  NSM server just
     send all of service bits which server can provide to the client.
     It is up to client side to determine the service is enough or not.  */
  ns->service.cindex = 0;
  ns->service.client_id = nse->service.client_id;

  /* When the client is graceful restart client, set forwarding
     preserve status.  */
  restart = &ng->restart[nse->service.protocol_id];

  if (restart->t_preserve)
    {
      pal_time_t current;

      ns->service.restart[AFI_IP][SAFI_UNICAST] = 1;
#ifdef HAVE_IPV6
      ns->service.restart[AFI_IP6][SAFI_UNICAST] = 1;
#endif /* HAVE_IPV6 */
      NSM_SET_CTYPE (ns->service.cindex, NSM_SERVICE_CTYPE_RESTART);

      /* Grace period expires.  */
      current = pal_time_current (NULL);
      ns->service.grace_period
        = restart->preserve_time - (current - restart->disconnect_time);

      if (ns->service.grace_period > 0)
        NSM_SET_CTYPE (ns->service.cindex, NSM_SERVICE_CTYPE_GRACE_EXPIRES);

      /* Restart option set.  */
      if (restart->restart_length)
        {
          NSM_SET_CTYPE (ns->service.cindex, NSM_SERVICE_CTYPE_RESTART_OPTION);
          ns->service.restart_val = restart->restart_val;
          ns->service.restart_length = restart->restart_length;
        }
    }

#ifdef HAVE_MCAST_IPV4
  /* Check for multicast service */
  if ((NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_IPV4_MRIB)) ||
      (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_IPV4_PIM)))
    nsm_mcast_recv_service (ns, nse);
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
  /* Check for multicast service */
  if ((NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_IPV6_MRIB)) ||
      (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_IPV6_PIM)))
    nsm_mcast6_recv_service (ns, nse);
#endif /* HAVE_MCAST_IPV6 */

#ifdef HAVE_L3
  ns->service.restart_state = nsm_restart_state (nse->service.protocol_id);
#endif /* HAVE_L3 */

#ifdef HAVE_MPLS
  if (ns->service.restart_state)
    nsm_lp_server_get_stale_for (nm,
                                 &(ns->service),
                                 nse->service.protocol_id,
                                 &(ns->service.label_pools),
                                 &(ns->service.label_pool_num));
#endif
#ifdef HAVE_L3
  /* Send SERVICE REPLY message to client.  */
  ns->service.restart_state = nsm_restart_state (nse->service.protocol_id);
#endif /* HAVE_L3 */
  ret = nsm_server_send_service (nse, header->message_id, &ns->service);
  if (ret < 0)
    return ret;

#ifdef HAVE_MPLS
  if (restart->t_preserve)
    if (nse->service.protocol_id == APN_PROTO_LDP ||
        nse->service.protocol_id == APN_PROTO_RSVP)
      {
#ifdef HAVE_RESTART
        if (nse->service.protocol_id == APN_PROTO_LDP)
          nsm_gmpls_ilm_owner_stale_sync(nm, nse->service.protocol_id);
#endif /* HAVE_RESTART */
        nsm_restart_stop (nse->service.protocol_id);
      }
#endif /* HAVE_MPLS */

#ifdef HAVE_L3
  /* Clear restart option.  */
  if (restart->restart_length)
    nsm_restart_option_unset (nse->service.protocol_id);
#endif /* HAVE_L3 */

  /* Send all NSM information to all VRs. */
  for (i = 0; i < vector_max (nzg->vr_vec); i++)
    if ((vr = vector_slot (nzg->vr_vec, i)))
      if ((ret = nsm_server_send_sync_vr (vr->proto, nse)) < 0)
        return ret;

#ifdef HAVE_L3
  /* BGP Protocol is up, send message to ISIS */
  if (nse->service.protocol_id == APN_PROTO_BGP)
    nsm_server_send_bgp_up (nse);

  /* LDP Protocol is up, send message to ISIS */
  if (nse->service.protocol_id == APN_PROTO_LDP)
    nsm_server_send_ldp_up (nse, header->vr_id, header->vrf_id);
#endif /* HAVE_L3 */

#ifdef HAVE_BFD
  /* OAM module is UP, establish connection with OAM server */
  if (nse->service.protocol_id == APN_PROTO_OAM)
  nsm_bfd_init (NSM_ZG);
#endif /* HAVE_BFD */

  return 0;
}

#ifdef HAVE_L3
void nsm_redistribute (struct nsm_vrf *, u_char, afi_t,
                       struct nsm_server_entry *);
void nsm_redistribute_default (struct nsm_vrf *, afi_t,
                               struct nsm_server_entry *);

/* Receive redistribute request.  */
s_int32_t
nsm_server_recv_redistribute_set (struct nsm_msg_header *header,
                                  void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse = arg;
  struct nsm_server *ns = nse->ns;
  struct nsm_msg_redistribute *msg = message;
  struct nsm_redistribute *redist;
  struct nsm_vrf *vrf;

  if (IS_NSM_DEBUG_RECV)
    nsm_redistribute_dump (ns->zg, msg);

  /* If type is bigger than max.  */
  if (msg->type >= APN_ROUTE_MAX)
    return 0;

  /* Lookup existing redistribute configuration from this client.  */
  for (redist = nse->redist; redist; redist = redist->next)
     {
       if (redist->type == msg->type && redist->afi == msg->afi
           && redist->vr_id == header->vr_id && redist->vrf_id == header->vrf_id)
         {
           if ((msg->afi == 0 || msg->afi == AFI_IP))
             {
               if (!NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_IPV4_PROCESS_ID))
                 break;
               else if (CHECK_FLAG (redist->flag, ENABLE_INSTANCE_ID)
                        &&  redist->pid == msg->pid)
                 break;
             }

#ifdef HAVE_IPV6
           if ((msg->afi == 0 || msg->afi == AFI_IP6))
             {
               if (!NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_IPV6_PROCESS_ID))
                 break;
               else if (CHECK_FLAG (redist->flag, ENABLE_INSTANCE_ID)
                        &&  redist->pid == msg->pid)
                 break;
             }
#endif /* HAVE_IPV6 */
        }
     }

  /* When alredy have the configuration, just return.  */
  if (redist)
    return 0;

  /* Add configuration to client.  */
  redist = XCALLOC (MTYPE_NSM_REDISTRIBUTE, sizeof (struct nsm_redistribute));
  redist->type = msg->type;
  redist->afi = msg->afi;
  redist->vr_id = header->vr_id;
  redist->vrf_id = header->vrf_id;
  SLIST_ADD (nse, redist, redist);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_IPV4_PROCESS_ID))
    {
      redist->pid = msg->pid;
      SET_FLAG(redist->flag, ENABLE_INSTANCE_ID);
    }
#ifdef HAVE_IPV6
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_IPV6_PROCESS_ID))
    {
      redist->pid = msg->pid;
      SET_FLAG(redist->flag, ENABLE_INSTANCE_ID);
    }
#endif /* HAVE_IPV6 */

  /* Get VR context. */
  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Set configuration to VRF.  */
  vrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (vrf)
    {
      /* AFI 0 means both IPv4 and IPv6.  */
      if (msg->afi == 0 || msg->afi == AFI_IP)
        {
          vector_set (vrf->afi[AFI_IP].redist[msg->type], nse);
          if (msg->type == APN_ROUTE_DEFAULT)
            nsm_redistribute_default (vrf, AFI_IP, nse);
          else
            nsm_redistribute (vrf, msg->type, AFI_IP, nse);
        }
#ifdef HAVE_IPV6
      if (NSM_CAP_HAVE_IPV6)
        if (msg->afi == 0 || msg->afi == AFI_IP6)
          {
            vector_set (vrf->afi[AFI_IP6].redist[msg->type], nse);
            if (msg->type == APN_ROUTE_DEFAULT)
              nsm_redistribute_default (vrf, AFI_IP6, nse);
            else
              nsm_redistribute (vrf, msg->type, AFI_IP6, nse);
          }
#endif /* HAVE_IPV6 */
    }

  return 0;
}

s_int32_t
nsm_server_recv_redistribute_unset (struct nsm_msg_header *header,
                                    void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse = arg;
  struct nsm_server *ns = nse->ns;
  struct nsm_msg_redistribute *msg = message;
  struct nsm_redistribute *redist;
  struct nsm_vrf *vrf;
  u_int32_t i;
  vector vec;

  if (IS_NSM_DEBUG_RECV)
    nsm_redistribute_dump (ns->zg, msg);

  /* If type is bigger than max.  */
  if (msg->type >= APN_ROUTE_MAX)
    return 0;

  /* Lookup existing redistribute configuration from this client.  */
  for (redist = nse->redist; redist; redist = redist->next)
     {
       if (redist->type == msg->type && redist->afi == msg->afi
          && redist->vr_id == header->vr_id && redist->vrf_id == header->vrf_id)
        {
         if (msg->afi == AFI_IP)
           {
             if (!NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_IPV4_PROCESS_ID))
               break;
             else if (CHECK_FLAG (redist->flag, ENABLE_INSTANCE_ID)
                      &&  redist->pid == msg->pid)
               break;
           }

#ifdef HAVE_IPV6
         if (msg->afi == AFI_IP6)
           {
             if (!NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_IPV6_PROCESS_ID))
               break;
             else if (CHECK_FLAG (redist->flag, ENABLE_INSTANCE_ID)
                      &&  redist->pid == msg->pid)
               break;
           }
#endif /* HAVE_IPV6 */
       }
    }

  if (! redist)
    return 0;

  /* Get VR context. */
  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Clear existing information.  */
  vrf = nsm_vrf_lookup_by_id (nm, redist->vrf_id);
  if (vrf)
    {
      if (redist->afi == 0 || redist->afi == AFI_IP)
        {
          vec = vrf->afi[AFI_IP].redist[redist->type];
          for (i = 0; i < vector_max (vec); i++)
            {
            if (vector_slot (vec, i) == nse)
              {
               vector_unset (vec, i);
               break;
              }
            }
        }

#ifdef HAVE_IPV6
      if (redist->afi == 0 || redist->afi == AFI_IP6)
        {
          vec = vrf->afi[AFI_IP6].redist[redist->type];
          for (i = 0; i < vector_max (vec); i++)
            if (vector_slot (vec, i) == nse)
              {
                vector_unset (vec, i);
                break;
              }
        }
#endif /* HAVE_IPV6 */
    }

  SLIST_DEL (nse, redist, redist);
  XFREE (MTYPE_NSM_REDISTRIBUTE, redist);

  return 0;
}

int
nsm_server_recv_isis_wait_bgp_set (struct nsm_msg_header *header,
                                   void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse = arg;
  struct nsm_msg_wait_for_bgp *msg = message;
  struct apn_vr *vr;
  struct nsm_vrf *vrf;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *bgp_nse;
  s_int32_t i;

  nm = NULL;
  nsc = NULL;
  bgp_nse = NULL;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  vrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);

  if (!vrf)
   return 0;

  vr = vrf->vrf->vr;

  /* check if BGP protocol has already converged if it is running */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, bgp_nse)
  if (bgp_nse->service.protocol_id == APN_PROTO_BGP)
    {
      if (bgp_nse->nsc->conv_state == PAL_TRUE)
        {
          nsm_server_send_bgp_conv_done (vr->id, vrf->vrf->id, nse, 0, msg);
          return 0;
        }
      else
        {
          return 0;
        }
    }

  /* Send Message to ISIS that BGP protocol daemon is down */
  nsm_server_send_message (nse, vr->id, vrf->vrf->id,
                           NSM_MSG_ISIS_BGP_DOWN, 0,1);

  return 0;
}

s_int32_t
nsm_server_recv_bgp_conv_done (struct nsm_msg_header *header,
                               void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_msg_wait_for_bgp *msg = message;
  struct apn_vr *vr;
  struct nsm_vrf *vrf;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *isis_nse;
  struct nsm_server_entry *bgp_nse;
  s_int32_t i;
  u_int16_t state;

  nm = NULL;
  nsc = NULL;
  isis_nse = NULL;
  bgp_nse = NULL;
  state = PAL_FALSE;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  vrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);

  if (!vrf)
   return 0;

  vr = vrf->vrf->vr;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, bgp_nse)
  if (bgp_nse->service.protocol_id == APN_PROTO_BGP)
    {
      bgp_nse->nsc->conv_state = PAL_TRUE;
      state = bgp_nse->nsc->conv_state;
    }

  if (state == PAL_TRUE)
    {
      /* send message to ISIS protocol module */
      NSM_SERVER_CLIENT_LOOP (ng, i, nsc, isis_nse)
      if (isis_nse->service.protocol_id == APN_PROTO_ISIS)
        nsm_server_send_bgp_conv_done (vr->id, vrf->vrf->id, isis_nse, 0,msg);
    }

  return 0;
}

/* send BGP converged message to ISIS */
s_int32_t
nsm_server_send_bgp_conv_done (u_int32_t vr_id, vrf_id_t vrf_id,
                               struct nsm_server_entry *nse, u_int32_t msg_id,
                               struct nsm_msg_wait_for_bgp *msg)
{
  s_int32_t nbytes = 0;

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_bgp_conv (&nse->send.pnt, &nse->send.size, msg);

  if (IS_NSM_DEBUG_SEND)
    zlog_info (nse->ns->zg, "NSM send BGP converge completed update to ISIS");

  nsm_server_send_message (nse, vr_id, vrf_id, NSM_MSG_ISIS_BGP_CONV_DONE,
                           msg_id, nbytes);

  return 0;
}

void
nsm_server_send_bgp_up (struct nsm_server_entry *nse)
{
  s_int32_t i;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *isis_nse;

  isis_nse = NULL;
  nsc = NULL;

  if (nse == NULL)
    return;

  /* send message to ISIS protocol module */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, isis_nse)
  if (isis_nse->service.protocol_id == APN_PROTO_ISIS)
    nsm_server_send_message (isis_nse, 0, 0, NSM_MSG_ISIS_BGP_UP, 0,1);

  return;
}

void
nsm_server_send_bgp_down (struct nsm_server_entry *nse)
{
  s_int32_t i;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *isis_nse;

  isis_nse = NULL;
  nsc = NULL;

  if (nse == NULL)
    return;

   /* send message to ISIS protocol module */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, isis_nse)
  if (isis_nse->service.protocol_id == APN_PROTO_ISIS)
    nsm_server_send_message (isis_nse, 0, 0, NSM_MSG_ISIS_BGP_DOWN, 0,1);

  return;
}

void
nsm_server_send_ldp_up (struct nsm_server_entry *nse,  u_int32_t vr_id, 
                        u_int32_t vrf_id)
{
  s_int32_t i;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *isis_nse;

  isis_nse = NULL;
  nsc = NULL;

  if (nse == NULL)
    return;

  /* send message to ISIS protocol module */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, isis_nse)
  if (isis_nse->service.protocol_id == APN_PROTO_ISIS)
    nsm_server_send_message (isis_nse, vr_id, vrf_id, NSM_MSG_LDP_UP, 0,1);

  return;
}

void
nsm_server_send_ldp_down (struct nsm_server_entry *nse, u_int32_t vr_id, 
                          u_int32_t vrf_id)
{
  s_int32_t i;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *isis_nse;

  isis_nse = NULL;
  nsc = NULL;

  if (nse == NULL)
    return;

   /* send message to ISIS protocol module */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, isis_nse)
  if (isis_nse->service.protocol_id == APN_PROTO_ISIS)
    nsm_server_send_message (isis_nse, vr_id, vrf_id, NSM_MSG_LDP_DOWN, 0,1);

  return;
}

s_int32_t
nsm_server_send_ldp_session_state (u_int32_t vr_id, vrf_id_t vrf_id,
                                   struct nsm_server_entry *nse, 
                                   u_int32_t msg_id,
                                   struct nsm_msg_ldp_session_state *msg)
{
  s_int32_t nbytes = 0;

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_ldp_session_state (&nse->send.pnt, &nse->send.size, msg);

  if (IS_NSM_DEBUG_SEND)
    zlog_info (nse->ns->zg, "NSM relay LDP session state = %d ifindex = %d",
               msg->state, msg->ifindex);

  nsm_server_send_message (nse, vr_id, vrf_id, NSM_MSG_LDP_SESSION_STATE,
                           msg_id, nbytes);

  return 0;
}

s_int32_t
nsm_server_send_ldp_session_query (u_int32_t vr_id, vrf_id_t vrf_id,
                                   struct nsm_server_entry *nse, 
                                   u_int32_t msg_id,
                                   struct nsm_msg_ldp_session_state *msg)
{
  s_int32_t nbytes = 0;

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_ldp_session_state (&nse->send.pnt, &nse->send.size, msg);

  if (IS_NSM_DEBUG_SEND)
    zlog_info (nse->ns->zg, "NSM relay LDP session query for ifindex = %d, %d",
               msg->ifindex, nbytes);

  nsm_server_send_message (nse, vr_id, vrf_id, NSM_MSG_LDP_SESSION_QUERY,
                           msg_id, nbytes);

  return 0;
}

int
nsm_server_recv_ldp_session_query (struct nsm_msg_header *header,
                                   void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse = arg;
  struct nsm_msg_ldp_session_state *msg = message;
  struct apn_vr *vr;
  struct nsm_vrf *vrf;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *ldp_nse;
  s_int32_t i;

  nm = NULL;
  nsc = NULL;
  ldp_nse = NULL;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  vrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);

  if (!vrf)
   return 0;

  vr = vrf->vrf->vr;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, ldp_nse)
    if (ldp_nse->service.protocol_id == APN_PROTO_LDP)
      {
        /* No support for LDP graceful restart yet */
        nsm_server_send_ldp_session_query (vr->id, vrf->vrf->id, ldp_nse, 0, 
                                           msg);
        return 0;
      }

  /* Send Message to ISIS that LDP protocol daemon is down */
  nsm_server_send_message (nse, vr->id, vrf->vrf->id,
                           NSM_MSG_LDP_DOWN, 0,1);

  return 0;
}

int
nsm_server_recv_ldp_session_state (struct nsm_msg_header *header,
                                   void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_msg_ldp_session_state *msg = message;
  struct apn_vr *vr;
  struct nsm_vrf *vrf;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *isis_nse;
  s_int32_t i;

  nm = NULL;
  nsc = NULL;
  isis_nse = NULL;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  vrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);

  if (!vrf)
   return 0;

  vr = vrf->vrf->vr;

  /* send message to ISIS protocol module */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, isis_nse)
  if (isis_nse->service.protocol_id == APN_PROTO_ISIS)
    nsm_server_send_ldp_session_state (vr->id, vrf->vrf->id, isis_nse, 0,msg);

  return 0;
}

#endif /* HAVE_L3 */

#ifdef HAVE_CRX
/* Receive interface address addition. */
s_int32_t
nsm_server_recv_interface_addr_add (struct nsm_msg_header *header,
                                    void *arg, void *message)
{
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_address *msg;

  nse = arg;
  ns = nse->ns;
  msg = message;

  if (IS_NSM_DEBUG_RECV)
    nsm_address_dump (ns->zg, msg);

  crx_vip_process_add (msg);

  return 0;
}

/* Receive interface address deletion. */
s_int32_t
nsm_server_recv_interface_addr_del (struct nsm_msg_header *header,
                                    void *arg, void *message)
{
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_address *msg;

  nse = arg;
  ns = nse->ns;
  msg = message;

  if (IS_NSM_DEBUG_RECV)
    nsm_address_dump (ns->zg, msg);

  crx_vip_process_delete (msg);

  return 0;
}

#endif /* HAVE_CRX */

#ifdef HAVE_L3
/* Receive route preserve message from client.  */
s_int32_t
nsm_server_recv_route_preserve (struct nsm_msg_header *header,
                                void *arg, void *message)
{
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_route_manage *msg;
  struct nsm_server_client *nsc;
  u_int32_t protocol_id;

  nse = arg;
  ns = nse->ns;
  nsc = nse->nsc;
  msg = message;

  /* Debug output.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_route_manage_dump (ns->zg, msg);

  /* If protocol ID is sent, use that or use the protocol_id from the
     connection. */
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_MANAGE_CTYPE_PROTOCOL_ID))
    protocol_id = msg->protocol_id;
  else
    protocol_id = nse->service.protocol_id;

#ifdef HAVE_L3
  /* Register protocol restart time.  */
  nsm_restart_register (protocol_id, msg->restart_time);

  /* If preserve message has restart option value, sture the
     value.  */
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_MANAGE_CTYPE_RESTART_OPTION))
    {
      nsm_restart_option_set (protocol_id, msg->restart_length,
                            msg->restart_val);
    }
#endif /* HAVE_L3 */

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Receive route preserve message from %s \n",
               modname_strl (protocol_id));

#ifdef HAVE_CRX
  /* Add protocol_id TLV. */
  NSM_SET_CTYPE (msg->cindex, NSM_ROUTE_MANAGE_CTYPE_PROTOCOL_ID);
  msg->protocol_id = protocol_id;
  crx_send_route_preserve (msg);
#endif /* HAVE_CRX */

  return 0;
}

/* Receive service request.  */
s_int32_t
nsm_server_recv_route_stale_remove (struct nsm_msg_header *header,
                                    void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_route_manage *msg;
#ifdef HAVE_MPLS
  bool_t ilm_force_rem_flag = NSM_FALSE;
#endif /*HAVE_MPLS*/

#ifdef HAVE_VRF
  struct apn_vr *vr;
  struct apn_vrf *iv;
  struct nsm_vrf *nv;
  s_int32_t i, j;
#endif /* HAVE_RESTART */

  nse = arg;
  ns = nse->ns;
  msg = message;

#ifdef HAVE_VRF
  vr = NULL;
  iv = NULL;
  nv = NULL;
  i = j = 0;
#endif /* HAVE_VRF */

  if (IS_NSM_DEBUG_RECV)
    nsm_route_manage_dump (ns->zg, msg);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);

  if (!nm)
    return -1;

#if defined HAVE_RESTART && defined HAVE_MPLS
  /* Process message */
  if (CHECK_FLAG (msg->flags, NSM_MSG_ILM_FORCE_STALE_DEL))
    ilm_force_rem_flag = NSM_TRUE;
#endif /* HAVE_RESTART && HAVE_MPLS */

  /* Remove stale route.  */
#ifdef HAVE_MPLS
  if (! msg->afi && ! msg->safi)
    {
      if (nse->service.protocol_id == APN_PROTO_LDP
       || nse->service.protocol_id == APN_PROTO_RSVP)
        {
          nsm_gmpls_restart_stale_remove (nm, nse->service.protocol_id,
                                          ilm_force_rem_flag);
        }
    }
#ifdef HAVE_VRF
   /* VPNV4 Graceful Restart, Stale entries should be removed from mpls rib
      as well as from BGP RIB */
  if ((msg->afi == AFI_IP)
       && (nse->service.protocol_id == APN_PROTO_BGP))
    {
      /* MPLS RIB stale Remove */
         nsm_gmpls_restart_stale_remove_vrf (nm, nse->service.protocol_id);
      /* NSM RIB stale remove */
         for (i = 0; i < vector_max (nm->vr->vrf_vec); i++)
           if ((iv = vector_slot (nm->vr->vrf_vec, i)))
             if ((nv = iv->proto))
               nsm_restart_stale_remove_vrf (nv, msg->afi,
                                             nse->service.protocol_id);
    }
  else
#endif /* HAVE_VRF */
#endif /*HAVE_MPLS*/
    nsm_restart_stale_remove (nm, nse->service.protocol_id, msg->afi, msg->safi);
  nsm_restart_state_unset (nse->service.protocol_id);

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Received Stale route remove message from %s \n",
                       modname_strl (nse->service.protocol_id));

  return 0;
}

/* Receive service request.  */
#ifdef HAVE_RESTART
s_int32_t
nsm_server_recv_route_stale_mark (struct nsm_msg_header *header,
                                    void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_route_manage *msg;

#ifdef HAVE_VRF
  struct apn_vr *vr;
  struct apn_vrf *iv;
  struct nsm_vrf *nv;
  s_int32_t i, j;
#endif /* HAVE_VRF */

  nse = arg;
  ns = nse->ns;
  msg = message;

#ifdef HAVE_VRF
  vr = NULL;
  iv = NULL;
  nv = NULL;
  i = j = 0;
#endif /* HAVE_VRF */

  if (IS_NSM_DEBUG_RECV)
    nsm_route_manage_dump (ns->zg, msg);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);

  if (!nm)
    return -1;

  /* Mark Vpnv4 routes as stale in MPLS RIB as well as in NSM RIB */
#ifdef HAVE_MPLS
  if (! msg->afi && ! msg->safi)
    {
      if (nse->service.protocol_id == APN_PROTO_LDP)
        {
          if (CHECK_FLAG (msg->flags, NSM_MSG_HELPER_NODE_STALE_MARK))
            {
              nsm_gmpls_stale_entries_update (nm, nse->service.protocol_id, 
                                              NSM_MARK_STALE_ENTRIES, 
                                              msg->oi_ifindex);
            }
          else if (CHECK_FLAG (msg->flags, NSM_MSG_HELPER_NODE_STALE_UNMARK))
            {
              nsm_gmpls_stale_entries_update (nm, nse->service.protocol_id, 
                                              NSM_UNMARK_STALE_ENTRIES, 
                                              msg->oi_ifindex);
            }
        }
      return 0;  
    }
#endif /*HAVE_MPLS*/
#ifdef HAVE_VRF
  if ((nse->service.protocol_id == APN_PROTO_BGP)
       && (msg->afi == AFI_IP))
    {
      /* MPLS RIB preserve */
      nsm_mpls_vrf_restart_stale_mark (nm, nse->service.protocol_id);
      /* NSM RIB preserve */
      for (i = 0; i < vector_max (nm->vr->vrf_vec); i++)
         if ((iv = vector_slot (nm->vr->vrf_vec, i)))
           if ((nv = iv->proto))
              nsm_restart_stale_mark_vrf (nv, msg->afi, nse->service.protocol_id);
    }
  else
#endif /* HAVE_VRF */
    nsm_restart_stale_mark (nm, nse->service.protocol_id, msg->afi, msg->safi);

 nsm_restart_state_set (nse->service.protocol_id);

  return 0;
}
#endif /* HAVE_RESTART */
#ifdef HAVE_CRX
/* Route clean message. */
s_int32_t
nsm_server_recv_route_clean (struct nsm_msg_header *header,
                             void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_route_clean *msg;
  vrf_id_t vrf_id = MAIN_TABLE_INDEX;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_route_clean_dump (ns->zg, msg);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CLEAN_CTYPE_VRF_ID))
    vrf_id = msg->vrf_id;

  nm = nsm_master_lookup_by_id (nzg, 0);

  /* Clean up RIB for AFI/SAFI and route type. */
  nsm_rib_clean (nm, msg->afi, msg->safi, msg->route_type_mask, vrf_id);

  return 0;
}

/* Protocol restart message. */
s_int32_t
nsm_server_recv_protocol_restart (struct nsm_msg_header *header,
                                  void *arg, void *message)
{
  struct nsm_msg_protocol_restart *msg;

  msg = message;

  crx_protocol_restart (msg);

  return 0;
}
#endif /* HAVE_CRX */
#endif /* HAVE_L3 */

#ifdef HAVE_MPLS
/* Label pool.  */
s_int32_t
nsm_server_recv_label_pool_request (struct nsm_msg_header *header,
                                    void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_label_pool *msg;
  struct nsm_msg_label_pool reply;
  struct nsm_msg_pkt_block_list *blist = NULL;
  struct lp_serv_lset_block lsblk_ret;
  u_int16_t label_space;
  u_int32_t block;
  u_char protocol;
  u_int8_t cl_range_owner = 0;
  u_int8_t range_ret_data = LABEL_RANGE_RETURN_DATA_NONE;

  pal_mem_set (&reply, 0, sizeof (struct nsm_msg_label_pool));
  pal_mem_set (&lsblk_ret, 0, sizeof (struct lp_serv_lset_block));
  lsblk_ret.lset_blk_type = LSO_BLK_TYPE_NONE;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;
  label_space = msg->label_space;
  protocol = nse->service.protocol_id;
  cl_range_owner = msg->range_owner;

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LABEL_POOL_CTYPE_BLOCK_LIST))
    blist = msg->blk_list;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_label_pool_dump (ns->zg, msg);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  block = nsm_lp_server_gen_get_block (nm, label_space, protocol, blist,
                           &lsblk_ret, &range_ret_data,
                           cl_range_owner, GMPLS_LABEL_PACKET);

  pal_mem_set (&reply, 0, sizeof (struct nsm_msg_label_pool));

  /* Prepare reply.  */
  if ((msg->cindex == 0) ||
      (NSM_CHECK_CTYPE (msg->cindex, NSM_LABEL_POOL_CTYPE_LABEL_RANGE)))
    {
      reply.label_space = msg->label_space;
      reply.label_block = block;
      NSM_SET_CTYPE (reply.cindex, NSM_LABEL_POOL_CTYPE_LABEL_BLOCK);

      /* Indicate that the block allocation was based on a range */
      if (range_ret_data == LABEL_RANGE_BLOCK_ALLOC_SUCCESS)
        {
          NSM_SET_CTYPE (reply.cindex, NSM_LABEL_POOL_CTYPE_LABEL_RANGE);
          reply.range_owner = msg->range_owner;
          reply.min_label = nsm_lp_server_gen_get_range_label_value (nm,
                                          reply.label_space, 0, cl_range_owner,
                                          GMPLS_LABEL_PACKET);
          reply.max_label = nsm_lp_server_gen_get_range_label_value (nm,
                                          reply.label_space, 1, cl_range_owner,
                                          GMPLS_LABEL_PACKET);
        }
      else
        {
          reply.min_label = nsm_lp_server_gen_get_label_value (nm, label_space,
                                       0, cl_range_owner, GMPLS_LABEL_PACKET);
          reply.max_label = nsm_lp_server_gen_get_label_value (nm, label_space,
                                       1,  cl_range_owner, GMPLS_LABEL_PACKET);
        }
    }

  if ((NSM_CHECK_CTYPE (msg->cindex, NSM_LABEL_POOL_CTYPE_BLOCK_LIST)) &&
      (blist->blk_req_type == BLK_REQ_TYPE_GET_LS_BLOCK))
    {
      NSM_SET_CTYPE (reply.cindex, NSM_LABEL_POOL_CTYPE_BLOCK_LIST);

      if (lsblk_ret.lset_blk_type == INCL_BLK_TYPE)
        {
          reply.blk_list[INCL_BLK_TYPE].blk_req_type = BLK_REQ_TYPE_GET_LS_BLOCK;
          SET_FLAG (reply.blk_list[INCL_BLK_TYPE].action_type,
                                                    ACTION_TYPE_INCLUDE_RANGE);
          SET_FLAG (reply.blk_list[INCL_BLK_TYPE].action_type,
                                                    ACTION_TYPE_INCLUDE_LIST);
        }
      else if (lsblk_ret.lset_blk_type == EXCL_BLK_TYPE)
        {
          reply.blk_list[INCL_BLK_TYPE].blk_req_type = BLK_REQ_TYPE_GET_LS_BLOCK;
          SET_FLAG (reply.blk_list[EXCL_BLK_TYPE].action_type,
                                                    ACTION_TYPE_EXCLUDE_RANGE);
          SET_FLAG (reply.blk_list[EXCL_BLK_TYPE].action_type,
                                                    ACTION_TYPE_EXCLUDE_LIST);
        }

      /* Returns only one block at this time */
      reply.blk_list[lsblk_ret.lset_blk_type].blk_count    = 1;
      reply.blk_list[lsblk_ret.lset_blk_type].blk_req_type =
                                                     BLK_REQ_TYPE_GET_LS_BLOCK;
      reply.blk_list[lsblk_ret.lset_blk_type].lset_blocks  =
                                                        &(lsblk_ret.block_id);
    }

  /* Debug.  */
  if (IS_NSM_DEBUG_SEND)
    nsm_label_pool_dump (ns->zg, &reply);

  /* Send reply to client.  */
  nsm_server_send_label_pool (header->vr_id, nse, header->message_id,
                              &reply, NSM_MSG_LABEL_POOL_REQUEST);

  return 0;
}

s_int32_t
nsm_server_recv_label_pool_release (struct nsm_msg_header *header,
                                    void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_label_pool *msg;
  struct nsm_msg_label_pool reply;
  u_int16_t label_space;
  u_int32_t block;
  u_char protocol;
  s_int32_t ret;
  u_int8_t cl_range_owner = 0;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;
  label_space = msg->label_space;
  protocol = nse->service.protocol_id;
  cl_range_owner = msg->range_owner;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_label_pool_dump (ns->zg, msg);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Prepare reply.  */
  pal_mem_set (&reply, 0, sizeof (struct nsm_msg_label_pool));
  reply.label_space = msg->label_space;
  reply.cindex = 0;

  /* Get block.  */
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LABEL_POOL_CTYPE_LABEL_BLOCK))
    {
      block = msg->label_block;

      /* Let label pool manager know that this block is now free */
      ret = nsm_lp_server_gen_free_block (nm, label_space, block, protocol,
                                        cl_range_owner, GMPLS_LABEL_PACKET);
      if (ret < 0)
        {
          zlog_info (NSM_ZG, "Could not free block %d for label space %d "
                     "for protocol %s",
                     block, label_space, modname_strl (protocol));
          block = -1;
        }

      NSM_SET_CTYPE (reply.cindex, NSM_LABEL_POOL_CTYPE_LABEL_BLOCK);
      reply.label_block = block;
    }

  /* Debug.  */
  if (IS_NSM_DEBUG_SEND)
    nsm_label_pool_dump (ns->zg, &reply);

  /* Send reply to client.  */
  nsm_server_send_label_pool (header->vr_id, nse, header->message_id,
                              &reply, NSM_MSG_LABEL_POOL_RELEASE);

  return 0;
}


#ifdef HAVE_PACKET
s_int32_t
nsm_server_recv_ilm_lookup (struct nsm_msg_header *header, void *arg,
                            void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_ilm_lookup *msg;
  struct nsm_msg_ilm_lookup reply;
  struct ilm_entry *ilm_out;
  struct gmpls_gen_label lbl;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_ilm_lookup_dump (ns->zg, msg);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Prepare reply.  */
  pal_mem_set (&reply, 0, sizeof (struct nsm_msg_ilm_lookup));

  lbl.type = gmpls_entry_type_ip;
  lbl.u.pkt = msg->ilm_in_label;

  ilm_out = nsm_gmpls_ilm_lookup_by_owner (nm, msg->ilm_in_ix, &lbl,
                                           msg->owner);
  if (! ilm_out || ! ilm_out->xc || ! ilm_out->xc->nhlfe)
      zlog_info (NSM_ZG, "Failed to locate out ILM");
  else
    {
      /* reply.owner = ilm_out->owner.owner;
      reply.opcode = ilm_out->xc->nhlfe->opcode;
      reply.ilm_in_ix = ilm_out->key.iif_ix;
      reply.ilm_in_label = ilm_out->key.in_label;
      reply.ilm_out_ix = ilm_out->xc->nhlfe->key.u.ipv4.oif_ix;
      reply.ilm_out_label = ilm_out->xc->nhlfe->key.u.ipv4.out_label;
      reply.ilm_ix = ilm_out->ilm_ix; */
    }

  /* Debug.  */
  if (IS_NSM_DEBUG_SEND)
    nsm_ilm_lookup_dump (ns->zg, &reply);

  /* Send reply to client.  */
  nsm_server_send_ilm_lookup (header->vr_id, nse, header->message_id,
                              &reply, NSM_MSG_ILM_LOOKUP);

  return 0;
}
#endif /* HAVE_PACKET */

/* Send FTN reply message.  */
s_int32_t
nsm_server_send_ftn_reply (u_int32_t vr_id, vrf_id_t vrf_id,
                           struct nsm_server_entry *nse, u_int32_t msg_id,
                           struct ftn_ret_data *ret_data)
{
  s_int32_t len;

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_ftn_ret_data (&nse->send.pnt, &nse->send.size, ret_data);
  if (len < 0)
    return len;

  nsm_server_send_message (nse, vr_id, vrf_id,
                           NSM_MSG_MPLS_FTN_REPLY, msg_id, len);

  return 0;
}

/* Send MPLS notification message. */
s_int32_t
nsm_server_send_mpls_notification (u_int32_t vr_id, vrf_id_t vrf_id,
                                   struct nsm_server_entry *nse,
                                   u_int32_t msg_id,
                                   struct mpls_notification *mn)
{
  s_int32_t len;

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_mpls_notification (&nse->send.pnt, &nse->send.size, mn);
  if (len < 0)
    return len;

  nsm_server_send_message (nse, vr_id, vrf_id,
                           NSM_MSG_MPLS_NOTIFICATION, msg_id, len);

  return 0;
}

/* Send ILM reply message.  */
s_int32_t
nsm_server_send_ilm_reply (u_int32_t vr_id, vrf_id_t vrf_id,
                           struct nsm_server_entry *nse, u_int32_t msg_id,
                           struct ilm_ret_data *ret_data)
{
  s_int32_t len;

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_ilm_ret_data (&nse->send.pnt, &nse->send.size, ret_data);
  if (len < 0)
    return len;

  nsm_server_send_message (nse, vr_id, vrf_id,
                           NSM_MSG_MPLS_ILM_REPLY, msg_id, len);

  return 0;
}
#endif /* HAVE_MPLS */

#ifdef HAVE_GMPLS
s_int32_t
nsm_server_recv_ilm_gen_lookup (struct nsm_msg_header *header, void *arg,
                                void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_ilm_gen_lookup *msg;
  struct nsm_msg_ilm_gen_lookup reply;
  struct ilm_entry *ilm_out;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_ilm_lookup_gen_dump (ns->zg, msg);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Prepare reply.  */
  pal_mem_set (&reply, 0, sizeof (struct nsm_msg_ilm_gen_lookup));

  ilm_out = nsm_gmpls_ilm_lookup_by_owner (nm, msg->ilm_in_ix,
                                           &msg->ilm_in_label, msg->owner);
  if (! ilm_out || ! ilm_out->xc || ! ilm_out->xc->nhlfe)
      zlog_info (NSM_ZG, "Failed to locate out ILM");
  else
    {
      /* reply.owner = ilm_out->owner.owner;
      reply.opcode = ilm_out->xc->nhlfe->opcode;
      reply.ilm_in_ix = ilm_out->key.iif_ix;
      reply.ilm_in_label = ilm_out->key.in_label;
      reply.ilm_out_ix = ilm_out->xc->nhlfe->key.u.ipv4.oif_ix;
      reply.ilm_out_label = ilm_out->xc->nhlfe->key.u.ipv4.out_label;
      reply.ilm_ix = ilm_out->ilm_ix; */
    }

  /* Debug.  */
  if (IS_NSM_DEBUG_SEND)
    nsm_ilm_lookup_gen_dump (ns->zg, &reply);

  /* Send reply to client.  */
  nsm_server_send_ilm_gen_lookup (header->vr_id, nse, header->message_id,
                                  &reply, NSM_MSG_ILM_GEN_LOOKUP);

  return 0;
}


s_int32_t
nsm_server_send_ilm_gen_reply (u_int32_t vr_id, vrf_id_t vrf_id,
                               struct nsm_server_entry *nse, u_int32_t msg_id,
                               struct ilm_gen_ret_data *ret_data)
{
  s_int32_t len;

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_ilm_gen_ret_data (&nse->send.pnt, &nse->send.size,
                                       ret_data);
  if (len < 0)
    return len;

  nsm_server_send_message (nse, vr_id, vrf_id,
                           NSM_MSG_MPLS_ILM_GEN_REPLY, msg_id, len);

  return 0;
}


s_int32_t
nsm_server_send_ilm_bidir_reply (u_int32_t vr_id, vrf_id_t vrf_id,
                                 struct nsm_server_entry *nse, u_int32_t msg_id,
                                 struct ilm_bidir_ret_data *ret_data)
{
  s_int32_t len;

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_ilm_bidir_ret_data (&nse->send.pnt, &nse->send.size,
                                       ret_data);
  if (len < 0)
    return len;

  nsm_server_send_message (nse, vr_id, vrf_id, NSM_MSG_MPLS_BIDIR_ILM_REPLY,
                           msg_id, len);

  return 0;
}

s_int32_t
nsm_server_send_ftn_bidir_reply (u_int32_t vr_id, vrf_id_t vrf_id,
                                 struct nsm_server_entry *nse, u_int32_t msg_id,
                                 struct ftn_bidir_ret_data *ret_data)
{
  s_int32_t len;

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_ftn_bidir_ret_data (&nse->send.pnt, &nse->send.size,
                                       ret_data);
  if (len < 0)
    return len;

  nsm_server_send_message (nse, vr_id, vrf_id, NSM_MSG_MPLS_BIDIR_FTN_REPLY,
                           msg_id, len);

  return 0;
}

#endif /* HAVE_GMPLS */

#ifdef HAVE_MPLS_VC
s_int32_t
nsm_server_recv_vc_fib_add (struct nsm_msg_header *header,
                            void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct nsm_msg_vc_fib_add *vcfa;
  s_int32_t ret;
#ifdef HAVE_GMPLS
  struct interface *ifp = NULL;
#ifdef HAVE_MS_PW
  struct nsm_mpls_circuit *vc = NULL;
#endif /* HAVE_MS_PW */
#endif /* HAVE_GMPLS */

  /* Set connection and message */
  nse = arg;
  ns = nse->ns;
  vcfa = message;

  /* Dump message */
  if (IS_NSM_DEBUG_RECV)
    nsm_vc_fib_add_dump (ns->zg, vcfa);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

      /* Process message */
#ifdef HAVE_VPLS
  if (vcfa->vc_style != MPLS_VC_STYLE_MARTINI)
    {
#ifdef HAVE_GMPLS
      ifp = if_lookup_by_index (&nm->vr->ifm, vcfa->nw_if_ix);
      if (ifp == NULL)
        return NSM_ERR_INVALID_ARGS;

      vcfa->nw_if_ix = ifp->gifindex;
#endif /* HAVE_GMPLS */

      ret = nsm_vpls_fib_add_msg_process (nm, vcfa);
    }
  else
#endif /* HAVE_VPLS */
    {
#ifdef HAVE_GMPLS
      /* Change the interface index as necessary */
#ifdef HAVE_MS_PW
      /* For MS-PW, ac_index can be NULL, as it is populated based on the
       * NW interface of the "stitched" vc
       */
      if (vcfa->ac_if_ix == 0)
        {
          vc = nsm_mpls_l2_circuit_lookup_by_id (nm, vcfa->vc_id);
          if (! vc || !vc->ms_pw)
            return NSM_ERR_INVALID_ARGS;
        }
      else
#endif /* HAVE_MS_PW */
        {
          ifp = if_lookup_by_index (&nm->vr->ifm, vcfa->ac_if_ix);
          if (ifp == NULL)
            return NSM_ERR_INVALID_ARGS;

          vcfa->ac_if_ix = ifp->gifindex;
        }

      ifp = if_lookup_by_index (&nm->vr->ifm, vcfa->nw_if_ix);
      if (ifp == NULL)
        return NSM_ERR_INVALID_ARGS;

      vcfa->nw_if_ix = ifp->gifindex;
#endif /* HAVE_GMPLS */
      ret = nsm_mpls_vc_fib_add_msg_process (nm, vcfa);
    }

  if (ret < 0)
    {
      zlog_err (NSM_ZG, "VC FIB add processing failed for vc id");
    }

  return 0;
}

s_int32_t
nsm_server_recv_tunnel_info (struct nsm_msg_header *header,
                             void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct nsm_msg_vc_tunnel_info *vcfa;

  /* Set connection and message */
  nse = arg;
  ns = nse->ns;
  vcfa = message;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  if (NSM_MPLS)
    {
      if (vcfa->type == VC_TUNNEL_INFO_ADD)
        {
          NSM_MPLS->lsr_id = vcfa->lsr_id;
          NSM_MPLS->label_space = 0;
          NSM_MPLS->index = vcfa->index;
        }
      else
        {
          NSM_MPLS->lsr_id = 0;
          NSM_MPLS->label_space = 0;
          NSM_MPLS->index = 0;
        }
    }

  return 0;
}

s_int32_t
nsm_server_recv_vc_fib_del (struct nsm_msg_header *header,
                            void *arg, void *message)
{
  struct nsm_master *nm = NULL;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct nsm_msg_vc_fib_delete *vcfd;
  s_int32_t ret;

  /* Set connection and message */
  nse = arg;
  ns = nse->ns;
  vcfd = message;

  /* Dump message */
  if (IS_NSM_DEBUG_RECV)
    nsm_vc_fib_delete_dump (ns->zg, vcfd);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

      /* Process message */
#ifdef HAVE_VPLS
  if (vcfd->vc_style != MPLS_VC_STYLE_MARTINI)
    {
      ret = nsm_vpls_fib_delete_msg_process (nm, vcfd);
    }
  else
#endif /* HAVE_VPLS */
    {
      ret = nsm_mpls_vc_fib_del_msg_process (nm, vcfd);
    }

  if (ret < 0)
    {
      zlog_err (NSM_ZG, "VC FIB delete processing failed for VC Id");
    }

  return 0;
}

s_int32_t
nsm_server_recv_pw_status (struct nsm_msg_header *header,
                           void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct nsm_msg_pw_status *msg;
  s_int32_t ret;

  /* Set connection and message */
  nse = arg;
  ns = nse->ns;
  msg = message;

  /* Dump message */
  if (IS_NSM_DEBUG_RECV)
    nsm_mpls_pw_status_msg_dump (ns->zg, msg);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  ret = nsm_mpls_remote_pw_status_update (nm, msg);
  if (ret < 0)
    {
      zlog_err (NSM_ZG,
                "Remote PW Status update processing failed for vc-id %d",
                msg->vc_id);
    }

  return 0;
}

#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VPLS
s_int32_t
nsm_server_recv_vpls_mac_withdraw (struct nsm_msg_header *header,
                                   void *arg, void *message)
{
  s_int32_t ret;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct nsm_msg_vpls_mac_withdraw *vmw;
  struct nsm_master *nm;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  vmw = message;

  /* Dump message.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_vpls_mac_withdraw_dump (ns->zg, vmw);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Process message */
  ret = nsm_vpls_mac_withdraw_process (nm, vmw);
  if (ret < 0)
    zlog_err (NSM_ZG, "VPLS MAC Address Withdraw for VPLS Instance:%d "
              "failed with error=%d", vmw->vpls_id, ret);

  return 0;
}


#endif /* HAVE_VPLS */

#ifdef HAVE_GMPLS
s_int32_t
nsm_read_data_link (struct nsm_msg_header *header,
                    void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct gmpls_if *gmif;
  struct nsm_msg_data_link *msg;
  struct datalink *dl;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Get the GMPLS interface structure for the same */
  gmif = gmpls_if_get (nzg, header->vr_id);
  if (gmif == NULL)
    return NSM_FAILURE;

  dl = data_link_lookup_by_name (gmif, msg->name);
  if (dl == NULL)
    return NSM_FAILURE;

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_REMOTE_LINKID))
    {
      /* Learn only when unknown */
      if (dl->r_linkid.type == GMPLS_LINK_ID_TYPE_UNKNOWN)
        {
          nsm_api_data_link_remote_interface_id_set (header->vr_id, msg->name,
                                                     &msg->r_linkid);
        }
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_TE_GIFINDEX))
    {
      /* Only if the interface is not already part of the */
      if (dl->telink == NULL)
        {
          nsm_api_add_data_link_to_te_link (header->vr_id, msg->name,
                                            msg->name);
        }
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_STATUS))
    {
      nsm_api_data_link_status_set (header->vr_id, msg->name, msg->status);

      nsm_api_data_link_flags_set (header->vr_id, msg->name, msg->flags &
                                   (GMPLS_DATA_LINK_TRANSPARENT |
                                    GMPLS_DATA_LINK_LINKID_MISMATCH_LMP |
                                    GMPLS_DATA_LINK_PROP_MISMATCH_LMP));
    }

  return NSM_SUCCESS;
}

s_int32_t
nsm_read_data_link_sub (struct nsm_msg_header *header,
                        void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct gmpls_if *gmif;
  struct nsm_msg_data_link_sub *msg;
  struct datalink *dl;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Get the GMPLS interface structure for the same */
  gmif = gmpls_if_get (nzg, header->vr_id);
  if (gmif == NULL)
    return NSM_FAILURE;

  dl = data_link_lookup_by_name (gmif, msg->name);
  if (dl == NULL)
    return NSM_FAILURE;

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_REMOTE_LINKID))
    {
      /* Learn only when unknown */
      if (dl->r_linkid.type == GMPLS_LINK_ID_TYPE_UNKNOWN)
        {
          nsm_api_data_link_remote_interface_id_set (header->vr_id, msg->name,
                                                     &msg->r_linkid);
        }
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_TE_GIFINDEX))
    {
      /* Only if the interface is not already part of the */
      if (dl->telink == NULL)
        {
          nsm_api_add_data_link_to_te_link (header->vr_id, msg->tlname,
                                            msg->name);
        }
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_STATUS))
    {
      nsm_api_data_link_status_set (header->vr_id, msg->name, msg->status);
      nsm_api_data_link_flags_set (header->vr_id, msg->name, msg->flags);
    }

  return NSM_SUCCESS;
}

s_int32_t
nsm_read_te_link (struct nsm_msg_header *header,
                  void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct gmpls_if *gmif;
  struct nsm_msg_te_link *msg;
  struct telink *tl;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  if (IS_NSM_DEBUG_RECV)
    nsm_te_link_dump (nzg, msg);

  /* Get the GMPLS interface structure for the same */
  gmif = gmpls_if_get (nzg, header->vr_id);
  if (gmif == NULL)
    return NSM_FAILURE;

  tl = te_link_lookup_by_name (gmif, msg->name);
  if (tl == NULL)
    return NSM_FAILURE;

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_REMOTE_LINKID))
    {
      /* Learn only when unknown */
      if (tl->r_linkid.type == GMPLS_LINK_ID_TYPE_UNKNOWN)
        {
          nsm_api_te_link_remote_interface_id_set (header->vr_id, msg->name,
                                                   &msg->r_linkid);
        }
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_FLAG))
    {
      if (CHECK_FLAG (msg->flags, GMPLS_TE_LINK_LMP_STATUS_UP))
        {
          nsm_api_te_link_set_status (header->vr_id, msg->name,
                                      GMPLS_TE_LINK_LMP_STATUS_UP);
        }
      else
        {
          nsm_api_te_link_unset_status (header->vr_id, msg->name,
                                        GMPLS_TE_LINK_LMP_STATUS_UP);
        }
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_LINK_ID))
    {
      /* Should not happen. Link ID should always be set */
    }

  return NSM_SUCCESS;
}

#endif /* HAVE_GMPLS */

#ifdef HAVE_MPLS
/* Receive IPv4 FTN message.  */
#ifdef HAVE_PACKET
s_int32_t
nsm_server_recv_ftn_ipv4 (struct nsm_msg_header *header,
                          void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct ftn_add_data *fad;
  struct ftn_ret_data ret_data;
  s_int32_t ret;
#ifdef HAVE_GMPLS
  struct interface *ifp;
#endif /* HAVE_GMPLS */

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  fad = message;

  /* Dump message.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_ftn_ip_dump (ns->zg, fad);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Process message.  */
  if (CHECK_FLAG (fad->flags, NSM_MSG_FTN_ADD))
    {
#ifdef HAVE_GMPLS
      /* Change the interface index as necessary */
      ifp = if_lookup_by_index (&nm->vr->ifm, fad->oif_ix);
      if (ifp == NULL)
        return NSM_ERR_INVALID_ARGS;

      fad->oif_ix = ifp->gifindex;
#endif /* HAVE_GMPLS */

      ret = nsm_mpls_ftn_add_msg_process (nm, fad, &ret_data, MPLS_FTN_NONE,
                                          NSM_FALSE);
      if (ret == NSM_SUCCESS || ret == NSM_ERR_FTN_INSTALL_FAILURE)
        {
          if (ret == NSM_ERR_FTN_INSTALL_FAILURE)
            zlog_warn (NSM_ZG, "FTN FEC: %O failed to add to the forwarder\n",
                       &fad->fec_prefix);

          ret_data.id = fad->id;
          nsm_server_send_ftn_reply (header->vr_id, header->vrf_id, nse,
                                     header->message_id, &ret_data);
        }
      else
        {
          struct mpls_notification mn;

          zlog_warn (NSM_ZG, "FTN add processing failed with err = %d", ret);
          zlog_warn (NSM_ZG, "Sending notification");

          /* Fill notification. */
          pal_mem_set (&mn, 0, sizeof (struct mpls_notification));
          pal_mem_cpy (&mn.owner, &fad->owner, sizeof (struct mpls_owner));
          mn.id = fad->id;
          mn.type = MPLS_NTF_FTN_ADD_FAILURE;

          ret =
            nsm_server_send_mpls_notification (header->vr_id,
                                               header->vrf_id, nse,
                                               header->message_id, &mn);
        }
    }
  else if (CHECK_FLAG (fad->flags, NSM_MSG_FTN_FAST_DELETE))
    {
      /* Fast Delete. */
      ret = nsm_mpls_ftn_del_msg_process (nm, fad->vrf_id, &fad->fec_prefix,
                                          fad->ftn_ix);
      if (ret != NSM_SUCCESS)
        zlog_warn (NSM_ZG, "FTN delete processing failed, vrf = %d, "
                   "fec prefix = %O, ftn index = %d, err = %d",
                   fad->vrf_id, &fad->fec_prefix, fad->ftn_ix, ret);
    }
  else
    {
#ifdef HAVE_GMPLS
      /* Change the interface index as necessary */
      ifp = if_lookup_by_index (&nm->vr->ifm, fad->oif_ix);
      if (ifp == NULL)
        return NSM_ERR_INVALID_ARGS;

      fad->oif_ix = ifp->gifindex;
#endif /* HAVE_GMPLS */

      /* Slow Delete. */
      ret = nsm_mpls_ftn_del_slow_msg_process (nm, fad);
      if (ret != NSM_SUCCESS)
        zlog_warn (NSM_ZG, "FTN slow delete processing failed, err = %d",
                   ret);
    }

  return 0;
}

#endif /* HAVE_PACKET */

s_int32_t
nsm_server_recv_ftn_gen_ipv4 (struct nsm_msg_header *header,
                              void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct ftn_add_gen_data *fad;
  struct ftn_ret_data ret_data;
  s_int32_t ret;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  fad = message;

  /* Dump message.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_ftn_ip_gen_dump (ns->zg, fad);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Process message.  */
  if (CHECK_FLAG (fad->flags, NSM_MSG_GEN_FTN_ADD))
    {
      ret = nsm_gmpls_ftn_add_msg_process (nm, fad, &ret_data, MPLS_FTN_NONE,
                                           NSM_FALSE);
      if (ret == NSM_SUCCESS || ret == NSM_ERR_FTN_INSTALL_FAILURE)
        {
          if (ret == NSM_ERR_FTN_INSTALL_FAILURE)
            {
              if (fad->ftn.type == gmpls_entry_type_ip)
                {
                  zlog_warn (NSM_ZG,
                             "FTN FEC: %O failed to add to the forwarder\n",
                             &fad->ftn.u.prefix);
                }
            }

          if (CHECK_FLAG (fad->flags, NSM_MSG_GEN_FTN_BIDIR))
            {
              SET_FLAG (ret_data.flags, NSM_MSG_REPLY_TO_BIDIR);
            }

          if (CHECK_FLAG (fad->flags, NSM_MSG_GEN_FTN_FWD))
            {
              SET_FLAG (ret_data.flags, NSM_MSG_REPLY_TO_FWD);
            }

          ret_data.id = fad->id;
          nsm_server_send_ftn_reply (header->vr_id, header->vrf_id, nse,
                                     header->message_id, &ret_data);
        }
      else
        {
          struct mpls_notification mn;

          zlog_warn (NSM_ZG, "FTN add processing failed with err = %d", ret);
          zlog_warn (NSM_ZG, "Sending notification");

          /* Fill notification. */
          pal_mem_set (&mn, 0, sizeof (struct mpls_notification));
          pal_mem_cpy (&mn.owner, &fad->owner, sizeof (struct mpls_owner));
          mn.id = fad->id;
          mn.type = MPLS_NTF_FTN_ADD_FAILURE;

          ret =
            nsm_server_send_mpls_notification (header->vr_id,
                                               header->vrf_id, nse,
                                               header->message_id, &mn);
        }
    }
  else if (CHECK_FLAG (fad->flags, NSM_MSG_GEN_FTN_FAST_ADD))
    {
       /* Fast add is required in case the dummy FTN entry has already been
          installed */
      ret = nsm_gmpls_ftn_fast_add_msg_process (nm, header->vrf_id, &fad->ftn,
                                                fad->ftn_ix);

      if (ret == NSM_SUCCESS || ret == NSM_ERR_FTN_INSTALL_FAILURE)
        {
          if (ret == NSM_ERR_FTN_INSTALL_FAILURE)
            {
              if (fad->ftn.type == gmpls_entry_type_ip)
                {
                  zlog_warn (NSM_ZG,
                             "FTN FEC: %O failed to add to the forwarder\n",
                             &fad->ftn.u.prefix);
                }
            }

          ret_data.id = fad->id;
          nsm_server_send_ftn_reply (header->vr_id, header->vrf_id, nse,
                                     header->message_id, &ret_data);
        }
      else
        {
          struct mpls_notification mn;

          zlog_warn (NSM_ZG, "FTN add processing failed with err = %d", ret);
          zlog_warn (NSM_ZG, "Sending notification");

          /* Fill notification. */
          pal_mem_set (&mn, 0, sizeof (struct mpls_notification));
          pal_mem_cpy (&mn.owner, &fad->owner, sizeof (struct mpls_owner));
          mn.id = fad->id;
          mn.type = MPLS_NTF_FTN_ADD_FAILURE;

          ret =
            nsm_server_send_mpls_notification (header->vr_id,
                                               header->vrf_id, nse,
                                               header->message_id, &mn);
        }


    }
  else if (CHECK_FLAG (fad->flags, NSM_MSG_GEN_FTN_FAST_DEL))
    {
      /* Fast Delete. */
      ret = nsm_gmpls_ftn_del_msg_process (nm, fad->vrf_id, &fad->ftn,
                                           fad->ftn_ix);

      if (ret != NSM_SUCCESS)
        {
          if (fad->ftn.type == gmpls_entry_type_ip)
            {
               zlog_warn (NSM_ZG, "FTN delete processing failed, vrf = %d, "
                                  "fec prefix = %O, ftn index = %d, err = %d",
                                   fad->vrf_id, &fad->ftn.u.prefix,
                                   fad->ftn_ix, ret);
            }
        }
    }
  else if (CHECK_FLAG (fad->flags, NSM_MSG_GEN_FTN_DEL))
    {
      /* Slow Delete. */
      ret = nsm_gmpls_ftn_del_slow_msg_process (nm, fad);
      if (ret != NSM_SUCCESS)
        {
          zlog_warn (NSM_ZG, "FTN slow delete processing failed, err = %d",
                     ret);
        }
    }

  return 0;
}

s_int32_t
nsm_server_recv_ilm_gen_ipv4 (struct nsm_msg_header *header,
                              void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct ilm_add_gen_data *iad;
  struct ilm_gen_ret_data ret_data;
  struct ilm_ret_data oret_data;
  s_int32_t ret;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  iad = message;

  /* Dump message.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_ilm_ip_gen_dump (ns->zg, iad);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Process message.  */
  if (CHECK_FLAG (iad->flags, NSM_MSG_ILM_ADD))
    {
      ret = nsm_gmpls_ilm_add_msg_process (nm, iad, NSM_FALSE, &ret_data,
                                           NSM_FALSE);
      if (ret == NSM_SUCCESS || ret == NSM_ERR_ILM_INSTALL_FAILURE)
        {
          if (ret == NSM_ERR_ILM_INSTALL_FAILURE)
            {
              zlog_warn (NSM_ZG, "ILM FEC: %r failed to add to the forwarder",
                                    &iad->fec_prefix.u.prefix4);
            }

          ret_data.id = iad->id;
          if (CHECK_FLAG (iad->flags, NSM_MSG_ILM_TRANS_MPLS))
            {
              UNSET_FLAG (iad->flags, NSM_MSG_ILM_TRANS_MPLS);
              oret_data.flags = ret_data.flags;
              oret_data.id = ret_data.id;
              oret_data.owner = ret_data.owner;
              oret_data.iif_ix = ret_data.iif_ix;
              oret_data.in_label = ret_data.in_label.u.pkt;
              oret_data.ilm_ix = ret_data.ilm_ix;
              oret_data.xc_ix = ret_data.xc_ix;
              oret_data.nhlfe_ix = ret_data.nhlfe_ix;

              nsm_server_send_ilm_reply (header->vr_id, header->vrf_id, nse,
                                         header->message_id, &oret_data);
            }
          else
            {
#ifdef HAVE_GMPLS

              if (CHECK_FLAG (iad->flags, NSM_MSG_ILM_BIDIR))
                {
                  SET_FLAG (ret_data.flags, NSM_MSG_REPLY_TO_BIDIR);
                }

              if (CHECK_FLAG (iad->flags, NSM_MSG_GEN_ILM_FWD))
                {
                  SET_FLAG (ret_data.flags, NSM_MSG_REPLY_TO_FWD);
                }

              SET_FLAG (ret_data.flags, NSM_MSG_REPLY_TO_GEN_ILM_ADD);
              nsm_server_send_ilm_gen_reply (header->vr_id, header->vrf_id, nse,
                                             header->message_id, &ret_data);
#endif /*HAVE_GMPLS */
            }
        }
      else
        {
          struct mpls_notification mn;

          zlog_warn (NSM_ZG, "ILM add processing failed, err = %d", ret);
          zlog_warn (NSM_ZG, "Sending notification");

          /* Fill notification. */
          pal_mem_set (&mn, 0, sizeof (struct mpls_notification));
          pal_mem_cpy (&mn.owner, &iad->owner, sizeof (struct mpls_owner));
          mn.id = iad->id;
          mn.type = MPLS_NTF_ILM_ADD_FAILURE;

          ret =
            nsm_server_send_mpls_notification (header->vr_id,
                                               header->vrf_id, nse,
                                               header->message_id, &mn);
        }
    }
  else if (CHECK_FLAG (iad->flags, NSM_MSG_ILM_FAST_DELETE))
    {
      /* Fast Delete */
      ret = nsm_gmpls_ilm_fast_del_msg_process (nm, iad->iif_ix, &iad->in_label,
                                                iad->ilm_ix);
      if (ret != NSM_SUCCESS)
        zlog_warn (NSM_ZG, "ILM fast delete processing failed, err = %d", ret);
    }
  else
    {
      /* Delete */
      ret = nsm_gmpls_ilm_del_msg_process (nm, iad);
      if (ret != NSM_SUCCESS)
        zlog_warn (NSM_ZG, "ILM delete processing failed, err = %d", ret);
    }

  return 0;
}

#ifdef HAVE_PACKET
/* Receive IPv4 ILM message.  */
s_int32_t
nsm_server_recv_ilm_ipv4 (struct nsm_msg_header *header,
                          void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct ilm_add_data *iad;
  struct ilm_add_gen_data gen_data;
  struct ilm_ret_data ret_data;
  struct gmpls_gen_label lbl;
#ifdef HAVE_GMPLS
  struct interface *ifp;
#endif /* HAVE_GMPLS */
  s_int32_t ret;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  iad = message;

  /* Dump message.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_ilm_ip_dump (ns->zg, iad);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Process message.  */
  if (CHECK_FLAG (iad->flags, NSM_MSG_ILM_ADD))
    {
      ret = nsm_gmpls_ilm_add_msg_process_pkt (nm, iad, NSM_FALSE, &ret_data,
                                               NSM_FALSE);
      if (ret == NSM_SUCCESS || ret == NSM_ERR_ILM_INSTALL_FAILURE)
        {
          if (ret == NSM_ERR_ILM_INSTALL_FAILURE)
            zlog_warn (NSM_ZG, "ILM FEC: %r failed to add to the forwarder",
                       &iad->fec_prefix.u.prefix4);

          ret_data.id = iad->id;
          nsm_server_send_ilm_reply (header->vr_id, header->vrf_id, nse,
                                     header->message_id, &ret_data);
        }
      else
        {
          struct mpls_notification mn;

          zlog_warn (NSM_ZG, "ILM add processing failed, err = %d", ret);
          zlog_warn (NSM_ZG, "Sending notification");

          /* Fill notification. */
          pal_mem_set (&mn, 0, sizeof (struct mpls_notification));
          pal_mem_cpy (&mn.owner, &iad->owner, sizeof (struct mpls_owner));
          mn.id = iad->id;
          mn.type = MPLS_NTF_ILM_ADD_FAILURE;

          ret =
            nsm_server_send_mpls_notification (header->vr_id,
                                               header->vrf_id, nse,
                                               header->message_id, &mn);
        }
    }
  else if (CHECK_FLAG (iad->flags, NSM_MSG_ILM_FAST_DELETE))
    {
      /* Fast Delete */
#ifdef HAVE_GMPLS
      /* Change the interface index as necessary */
      ifp = if_lookup_by_index (&nm->vr->ifm, iad->iif_ix);
      if (ifp == NULL)
        return NSM_ERR_INVALID_ARGS;

      iad->iif_ix = ifp->gifindex;
#endif /* HAVE_GMPLS */

      lbl.type = gmpls_entry_type_ip;
      lbl.u.pkt = iad->in_label;
      ret = nsm_gmpls_ilm_fast_del_msg_process (nm, iad->iif_ix, &lbl,
                                                iad->ilm_ix);
      if (ret != NSM_SUCCESS)
        zlog_warn (NSM_ZG, "ILM fast delete processing failed, err = %d", ret);
    }
  else
    {
      /* Delete */
      nsm_gmpls_set_ilm_gen_data (&gen_data, iad);

#ifdef HAVE_GMPLS
      ifp = if_lookup_by_index (&nm->vr->ifm, iad->oif_ix);
      if (! ifp)
        return NSM_ERR_IF_NOT_FOUND;

      gen_data.oif_ix = ifp->gifindex;

      ifp = if_lookup_by_index (&nm->vr->ifm, iad->iif_ix);
      if (! ifp)
        return NSM_ERR_IF_NOT_FOUND;

      gen_data.iif_ix = ifp->gifindex;
#endif /* HAVE_GMPLS */


      ret = nsm_gmpls_ilm_del_msg_process (nm, &gen_data);
      if (ret != NSM_SUCCESS)
        zlog_warn (NSM_ZG, "ILM delete processing failed, err = %d", ret);
    }

  return 0;
}
#endif /* HAVE_PACKET */

void
nsm_gen_label_dump (struct lib_globals *zg, struct gmpls_gen_label *lbl,
                    u_char *pri_str, u_char *suf_str)
{
  switch (lbl->type)
    {
      case gmpls_entry_type_ip:
        zlog_info (zg, "\nPacket %s%u%s\n", pri_str, lbl->u.pkt, suf_str);
        break;

#ifdef HAVE_PBB_TE
       case gmpls_entry_type_pbb_te:
         zlog_info (zg, "\nPBB-TE %s %02x:%02x:%02x:%02x:%02x:%02x %d %s\n",
                    pri_str, lbl->u.pbb.bmac [0], lbl->u.pbb.bmac [1],
                             lbl->u.pbb.bmac [2], lbl->u.pbb.bmac [3],
                             lbl->u.pbb.bmac [4], lbl->u.pbb.bmac [5], suf_str);
         break;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
       case gmpls_entry_type_tdm:
         zlog_info (zg, "\nTDM %s %d %d %s", pri_str, lbl->u.tdm.gifindex,
                        lbl->u.tdm.tslot, suf_str);
         break;
#endif /* HAVE_TDM */
       default:
         break;
    }

}

void
nsm_gen_fec_dump (struct lib_globals *zg, struct fec_gen_entry *fec,
                  u_char *pri_str, u_char *suf_str)
{
  switch (fec->type)
    {
      case gmpls_entry_type_ip:
        zlog_info (zg, "\nPacket %s", pri_str);

      if (fec->u.prefix.family == AF_INET)
        zlog_info (zg, " FEC: %r %s\n", &fec->u.prefix.u.prefix4, suf_str);
#ifdef HAVE_IPV6
      else if (fec->u.prefix.family == AF_INET6)
        zlog_info (zg, " FEC: %R %s\n", &fec->u.prefix.u.prefix6, suf_str);
#endif /* HAVE_IPV6 */

        break;

#ifdef HAVE_PBB_TE
       case gmpls_entry_type_pbb_te:
         zlog_info (zg, "\nPBB-TE I-SID %s %u %s\n",
                    pri_str, fec->u.pbb.isid [2] + (fec->u.pbb.isid [2] * 255) +
                             (fec->u.pbb.isid [0] * 255 *255), suf_str);
         break;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
       case gmpls_entry_type_tdm:
         zlog_info (zg, "\nTDM %s %d %d %s", pri_str, fec->u.tdm.gifindex,
                        fec->u.tdm.tslot, suf_str);
         break;
#endif /* HAVE_TDM */
       default:
         break;
    }

}

void
nsm_ilm_gen_dump (struct lib_globals *zg, struct ilm_add_gen_data *iad)
{
  zlog_info (zg, "ILM IPv4 %s",
             CHECK_FLAG (iad->flags, NSM_MSG_ILM_ADD) ? "Add"
             : CHECK_FLAG (iad->flags, NSM_MSG_ILM_DELETE) ? "Delete"
             : "Fast Delete");

  if (CHECK_FLAG (iad->flags, NSM_MSG_ILM_ADD)
      || CHECK_FLAG (iad->flags, NSM_MSG_ILM_DELETE))
    nsm_mpls_owner_dump (zg, &iad->owner);

  zlog_info (zg, " In index: %u", iad->iif_ix);
  nsm_gen_label_dump (zg, &iad->in_label, "In label:", "");
  zlog_info (zg, " ILM index:%u", iad->ilm_ix);

  if (CHECK_FLAG (iad->flags, NSM_MSG_ILM_ADD)
      || CHECK_FLAG (iad->flags, NSM_MSG_ILM_DELETE))
    {
      nsm_gen_label_dump (zg, &iad->out_label, "Out label:", "");
      zlog_info (zg, " Out index: %u", iad->oif_ix);
      zlog_info (zg, " NH addr: %r", &iad->nh_addr.u.ipv4);

      if (iad->owner.owner == MPLS_RSVP)
         zlog_info (zg, " LSP ID: %x", iad->lspid.u.rsvp_lspid);
      else
         zlog_info (zg, " LSP ID: %x %x %x %x %x %x",
                          iad->lspid.u.cr_lspid[5], iad->lspid.u.cr_lspid[4],
                          iad->lspid.u.cr_lspid[3], iad->lspid.u.cr_lspid[2],
                          iad->lspid.u.cr_lspid[1], iad->lspid.u.cr_lspid[0]);

      zlog_info (zg, " # of pops: %d", iad->n_pops);

      zlog_info (zg, "FEC %O", &iad->fec_prefix);

      zlog_info (zg, " QOS resource ID: %u", iad->qos_resrc_id);
      zlog_info (zg, " Opcode: %s", nsm_mpls_dump_op_code (iad->opcode));
      zlog_info (zg, " Primary ILM entry: %d", iad->primary);
    }
}

#ifdef HAVE_GMPLS
/* Receive Generalized ILM message.  */
s_int32_t
nsm_server_recv_gmpls_ilm (struct nsm_msg_header *header,
                           void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct ilm_add_gen_data *iad;
  struct ilm_gen_ret_data ret_data;
  s_int32_t ret;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  iad = message;

  SET_FLAG (iad->flags, NSM_MSG_ILM_GEN_MPLS);
  /* Dump message.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_ilm_gen_dump (ns->zg, iad);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Process message.  */
  if (CHECK_FLAG (iad->flags, NSM_MSG_ILM_ADD))
    {

      ret = nsm_gmpls_ilm_add_msg_process (nm, iad, NSM_FALSE, &ret_data,
                                           NSM_FALSE);

      if (ret == NSM_SUCCESS || ret == NSM_ERR_ILM_INSTALL_FAILURE)
        {
          if (ret == NSM_ERR_ILM_INSTALL_FAILURE)
            {
#ifdef HAVE_PACKET
              zlog_warn (NSM_ZG, "ILM FEC: %r failed to add to the forwarder",
                         &iad->fec_prefix.u.prefix4);
#endif /* HAVE_PACKET */
            }

          if (CHECK_FLAG (iad->flags, NSM_MSG_ILM_BIDIR))
            {
              SET_FLAG (ret_data.flags, NSM_MSG_REPLY_TO_BIDIR);
            }

          if (CHECK_FLAG (iad->flags, NSM_MSG_GEN_ILM_FWD))
            {
              SET_FLAG (ret_data.flags, NSM_MSG_REPLY_TO_FWD);
            }

          ret_data.id = iad->id;
          nsm_server_send_ilm_gen_reply (header->vr_id, header->vrf_id, nse,
                                         header->message_id, &ret_data);
        }
      else
        {
          struct mpls_notification mn;

          zlog_warn (NSM_ZG, "ILM add processing failed, err = %d", ret);
          zlog_warn (NSM_ZG, "Sending notification");

          /* Fill notification. */
          pal_mem_set (&mn, 0, sizeof (struct mpls_notification));
          pal_mem_cpy (&mn.owner, &iad->owner, sizeof (struct mpls_owner));
          mn.id = iad->id;
          mn.type = MPLS_NTF_ILM_ADD_FAILURE;

          ret =
            nsm_server_send_mpls_notification (header->vr_id,
                                               header->vrf_id, nse,
                                               header->message_id, &mn);
        }
    }
  else if (CHECK_FLAG (iad->flags, NSM_MSG_ILM_FAST_DELETE))
    {
      /* Fast Delete */
      ret = nsm_gmpls_ilm_fast_del_msg_process (nm, iad->iif_ix, &iad->in_label,
                                                iad->ilm_ix);
      if (ret != NSM_SUCCESS)
        zlog_warn (NSM_ZG, "ILM fast delete processing failed, err = %d", ret);
    }
  else
    {
      /* Delete */
      ret = nsm_gmpls_ilm_del_msg_process (nm, iad);
      if (ret != NSM_SUCCESS)
        zlog_warn (NSM_ZG, "ILM delete processing failed, err = %d", ret);
    }

  return 0;
}
#endif /*HAVE_GMPLS */

#ifdef HAVE_PACKET
#ifdef HAVE_IPV6
s_int32_t
nsm_server_recv_ftn_ipv6 (struct nsm_msg_header *header,
                          void *arg, void *message)
{
  return nsm_server_recv_ftn_ipv4 (header, arg, message);
}

s_int32_t
nsm_server_recv_ilm_ipv6 (struct nsm_msg_header *header,
                          void *arg, void *message)
{
  return nsm_server_recv_ilm_ipv4 (header, arg, message);
}

#endif /* HAVE_IPV6 */
#endif /* HAVE_PACKET */

void
nsm_server_update_igp_shortcut_flag (struct nsm_vrf *vrf,
                                     struct nsm_msg_igp_shortcut_route *isroute,
                                     int isset)
{
  struct nsm_ptree_node *rn = NULL;
  struct rib *rib;
  static char *msg[] = { "unset", "set" };


  if (!vrf)
    return;

  rn = nsm_ptree_node_lookup (vrf->afi[AFI_IP].rib[SAFI_UNICAST],
                              (u_char *)&isroute->fec.u.prefix4,
                              isroute->fec.prefixlen);
  if (!rn)
    return;

  if (isset)
    isset = 1;
  for (rib = rn->info; rib; rib = rib->next)
    {
      if (isset)
        SET_FLAG (rib->pflags, NSM_ROUTE_HAVE_IGP_SHORTCUT);
      else
        UNSET_FLAG (rib->pflags, NSM_ROUTE_HAVE_IGP_SHORTCUT);
      if (IS_NSM_DEBUG_EVENT)
        zlog_info (NSM_ZG, "%r/%d: %s IGP shortcut flag (0x%08x)",
                   &isroute->fec.u.prefix4,
                   isroute->fec.prefixlen,
                   msg[isset], rib->pflags);
    }
}

s_int32_t
nsm_server_recv_igp_shortcut_route (struct nsm_msg_header *header,
                                    void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_vrf *vrf;
  struct nsm_server_entry *nse;
  struct nsm_msg_igp_shortcut_route *isroute;
  s_int32_t ret;

  /* Set connection and message. */
  nse = arg;
  ns = nse->ns;
  isroute = (struct nsm_msg_igp_shortcut_route *)message;

  /* Dump message. */
  if (IS_NSM_DEBUG_RECV)
    nsm_igp_shortcut_route_dump (ns->zg, isroute);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  vrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);

  /* Process message. */
  if (isroute->action == NSM_MSG_IGP_SHORTCUT_ADD)
    {
      ret = nsm_mpls_igp_shortcut_route_add (nm, isroute);

      if (ret == NSM_SUCCESS)
        nsm_server_update_igp_shortcut_flag (vrf, isroute, 1);
      else
        {
          switch (ret)
            {
            case NSM_FAILURE:
              zlog_err (ns->zg, "%% Could not map IGP-Shortcut %O to %r\n",
                       &isroute->fec, &isroute->t_endp);
              break;
            case NSM_ERR_MEM_ALLOC_FAILURE:
              zlog_err (ns->zg, "%% Memory allocation failure\n");
              break;
            default:
              break;
            }
        }
    }
  else if (isroute->action == NSM_MSG_IGP_SHORTCUT_DELETE)
    {
      ret = nsm_mpls_igp_shortcut_route_delete (nm, isroute);

      if (ret == NSM_SUCCESS)
        nsm_server_update_igp_shortcut_flag (vrf, isroute, 0);
      else
        zlog_err (ns->zg, "%% Could not delete IGP-Shortcup route %O\n",
                  &isroute->fec);
    }
  else
    {
      ret = nsm_mpls_igp_shortcut_route_update (nm, isroute);

      if (ret != NSM_SUCCESS)
        zlog_err (ns->zg, "%% Could not update IGP-Shortcut route %O to %r\n",
                  &isroute->fec, &isroute->t_endp);
    }

  return 0;
}

#if defined HAVE_MPLS_OAM || defined HAVE_NSM_MPLS_OAM
s_int32_t
nsm_server_recv_mpls_vpn_rd (struct nsm_msg_header *header,
                             void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct nsm_msg_vpn_rd *msg, *temp;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;
  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  temp = XCALLOC (MTYPE_TMP, sizeof (struct nsm_msg_vpn_rd));
  pal_mem_cpy (temp, msg, sizeof (struct nsm_msg_vpn_rd));
  /* Now that we have the details we can store this value in the VPN tree */
  listnode_add (NSM_MPLS->mpls_vpn_list, temp);

  return NSM_SUCCESS;
}
#endif /* HAVE_MPLS_OAM || HAVE_NSM_MPLS_OAM */


#ifdef HAVE_GMPLS
s_int32_t
nsm_server_recv_gmpls_bidir_ftn (struct nsm_msg_header *header,
                                 void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct ftn_bidir_add_data *fad;
  struct ftn_bidir_ret_data ret_data;
  s_int32_t ret = 0;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  fad = message;

  pal_mem_set (&ret_data, 0, sizeof (struct ftn_bidir_ret_data));
  ret_data.id = fad->id;

  /* Dump message.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_ftn_bidir_ip_gen_dump (ns->zg, fad);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Process message.  */
  if (((CHECK_FLAG (fad->ftn.flags, NSM_MSG_GEN_FTN_ADD)) ||
      (CHECK_FLAG (fad->ftn.flags, NSM_MSG_GEN_FTN_DEL))  ||
      (CHECK_FLAG (fad->ftn.flags, NSM_MSG_GEN_FTN_FAST_DEL))) &&
      (CHECK_FLAG (fad->ftn.flags, NSM_MSG_GEN_FTN_BIDIR)) &&
      (fad->ftn.ftn.type == gmpls_entry_type_ip))
    {
      if (CHECK_FLAG (fad->ftn.flags, NSM_MSG_GEN_FTN_ADD))
        {
          ret = nsm_gmpls_ftn_add_msg_process (nm, &fad->ftn, &ret_data.ftn,
                                               MPLS_FTN_NONE, NSM_FALSE);
          if (ret == NSM_SUCCESS || ret == NSM_ERR_FTN_INSTALL_FAILURE)
            {
              if (ret == NSM_ERR_FTN_INSTALL_FAILURE)
                {
                  zlog_warn (NSM_ZG, "FTN FEC: %O failed to add"
                              " to the forwarder\n",
                                 &fad->ftn.ftn.u.prefix);
                }

              if (CHECK_FLAG (fad->ftn.flags, NSM_MSG_GEN_FTN_BIDIR))
                {
                  SET_FLAG (ret_data.ftn.flags, NSM_MSG_REPLY_TO_BIDIR);
                }

              if (CHECK_FLAG (fad->ftn.flags, NSM_MSG_GEN_FTN_FWD))
                {
                  SET_FLAG (ret_data.ftn.flags, NSM_MSG_REPLY_TO_FWD);
                }
            }
          else
            {
              struct mpls_notification mn;

              zlog_warn (NSM_ZG, "FTN add processing failed "
                                 "with err = %d", ret);
              zlog_warn (NSM_ZG, "Sending notification");

              /* Fill notification. */
              pal_mem_set (&mn, 0, sizeof (struct mpls_notification));
              pal_mem_cpy (&mn.owner, &fad->ftn.owner,
                           sizeof (struct mpls_owner));
              mn.id = fad->ftn.id;
              mn.type = MPLS_NTF_FTN_ADD_FAILURE;

              ret = nsm_server_send_mpls_notification (header->vr_id,
                                                       header->vrf_id, nse,
                                                       header->message_id, &mn);
              return 0;
            }

          ret_data.ftn.id = fad->ftn.id;
        }
      else if (CHECK_FLAG (fad->ftn.flags, NSM_MSG_GEN_FTN_DEL))
        {
          ret = nsm_gmpls_ftn_del_slow_msg_process (nm, &fad->ftn);
          if (ret != NSM_SUCCESS)
            {
              zlog_warn (NSM_ZG, "FTN slow delete processing failed, err = %d",
                         ret);
            }
        }
      else if (CHECK_FLAG (fad->ftn.flags, NSM_MSG_GEN_FTN_FAST_DEL))
        {
          /* Fast Delete. */
          ret = nsm_gmpls_ftn_del_msg_process (nm, fad->ftn.vrf_id,
                                               &fad->ftn.ftn, fad->ftn.ftn_ix);

          if (ret != NSM_SUCCESS)
            {
              if (fad->ftn.ftn.type == gmpls_entry_type_ip)
                {
                   zlog_warn (NSM_ZG, "FTN delete processing failed,"
                                      " vrf = %d, fec prefix = %O, ftn index "
                                      "= %d, err = %d", fad->ftn.vrf_id,
                                      &fad->ftn.ftn.u.prefix,
                                      fad->ftn.ftn_ix, ret);
                }
            }
        }
    }
  else
    {
      struct mpls_notification mn;

      zlog_warn (NSM_ZG, "Bidirectional FTN add failed = Flag Error");
      zlog_warn (NSM_ZG, "Sending notification");

      /* Fill notification. */
      pal_mem_set (&mn, 0, sizeof (struct mpls_notification));
      pal_mem_cpy (&mn.owner, &fad->ftn.owner, sizeof (struct mpls_owner));
      mn.id = fad->ftn.id;
      mn.type = MPLS_NTF_FTN_ADD_FAILURE;

      ret = nsm_server_send_mpls_notification (header->vr_id,
                                               header->vrf_id, nse,
                                               header->message_id, &mn);

      return 0;
    }

  /* Process message.  */
  if (((CHECK_FLAG (fad->ilm.flags, NSM_MSG_ILM_ADD)) ||
      (CHECK_FLAG (fad->ilm.flags, NSM_MSG_ILM_DELETE)) ||
      (CHECK_FLAG (fad->ilm.flags, NSM_MSG_ILM_FAST_DELETE))) &&
      (CHECK_FLAG (fad->ilm.flags, NSM_MSG_ILM_BIDIR)))
    {
      if (CHECK_FLAG (fad->ilm.flags, NSM_MSG_ILM_ADD))
        {
          fad->ilm.rev_xc_ix = ret_data.ftn.xc_ix;
          fad->ilm.rev_ftn_ix = ret_data.ftn.ftn_ix;
          ret = nsm_gmpls_ilm_add_msg_process (nm, &fad->ilm, NSM_FALSE,
                                               &ret_data.ilm, NSM_FALSE);
          if (ret == NSM_SUCCESS || ret == NSM_ERR_ILM_INSTALL_FAILURE)
            {
              if (ret == NSM_ERR_ILM_INSTALL_FAILURE)
                {
                  zlog_warn (NSM_ZG, "ILM FEC: %r failed to add"
                                     " to the forwarder",
                                    &fad->ilm.fec_prefix.u.prefix4);
                }

              if (CHECK_FLAG (fad->ilm.flags, NSM_MSG_ILM_BIDIR))
                {
                  SET_FLAG (ret_data.ilm.flags, NSM_MSG_REPLY_TO_BIDIR);
                }

              if (CHECK_FLAG (fad->ilm.flags, NSM_MSG_GEN_ILM_FWD))
                {
                  SET_FLAG (ret_data.ilm.flags, NSM_MSG_REPLY_TO_FWD);
                }

              ret_data.ilm.id = fad->ilm.id;
            }
          else
            {
              struct mpls_notification mn;

              zlog_warn (NSM_ZG, "ILM add processing failed, err = %d", ret);
              zlog_warn (NSM_ZG, "Sending notification");

              /* Fill notification. */
              pal_mem_set (&mn, 0, sizeof (struct mpls_notification));
              pal_mem_cpy (&mn.owner, &fad->ilm.owner,
                           sizeof (struct mpls_owner));
              mn.id = fad->ilm.id;
              mn.type = MPLS_NTF_ILM_ADD_FAILURE;

              ret = nsm_server_send_mpls_notification (header->vr_id,
                                                       header->vrf_id, nse,
                                                       header->message_id, &mn);
              return 0;
            }
        }
      else if (CHECK_FLAG (fad->ilm.flags, NSM_MSG_ILM_FAST_DELETE))
        {
          /* Fast Delete */
          ret = nsm_gmpls_ilm_fast_del_msg_process (nm, fad->ilm.iif_ix,
                                                    &fad->ilm.in_label,
                                                    fad->ilm.ilm_ix);
          if (ret != NSM_SUCCESS)
            zlog_warn (NSM_ZG, "ILM fast delete processing failed,"
                               " err = %d", ret);
        }
      else
        {
          ret = nsm_gmpls_ilm_del_msg_process (nm, &fad->ilm);
          if (ret != NSM_SUCCESS)
            zlog_warn (NSM_ZG, "ILM delete processing failed, err = %d", ret);

          /* Do not expect a reply for delete */
          return 0;
        }
    }
  else
    {
      struct mpls_notification mn;

      zlog_warn (NSM_ZG, "ILM add processing failed, err = %d", ret);
      zlog_warn (NSM_ZG, "Sending notification");

      /* Fill notification. */
      pal_mem_set (&mn, 0, sizeof (struct mpls_notification));
      pal_mem_cpy (&mn.owner, &fad->ilm.owner, sizeof (struct mpls_owner));
      mn.id = fad->ilm.id;
      mn.type = MPLS_NTF_ILM_ADD_FAILURE;

      ret = nsm_server_send_mpls_notification (header->vr_id, header->vrf_id,
                                               nse, header->message_id, &mn);
      return 0;
    }

  nsm_server_send_ftn_bidir_reply (header->vr_id, header->vrf_id, nse,
                                   header->message_id, &ret_data);
  return 0;
}

s_int32_t
nsm_server_recv_gmpls_bidir_ilm (struct nsm_msg_header *header,
                                 void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct ilm_bidir_add_data *iad;
  struct ilm_bidir_ret_data ret_data;
  struct ilm_add_gen_data *ilm;
  struct ilm_gen_ret_data *iret;
  s_int32_t ret = 0;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  iad = message;

  /* Dump message.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_ilm_bidir_ip_gen_dump (ns->zg, iad);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  pal_mem_set (&ret_data, '\0', sizeof (struct ilm_bidir_ret_data));
  ret_data.id = iad->id;
  ilm = &iad->ilm_fwd;
  iret = &ret_data.ilm_fwd;

  while (ilm != NULL)
    {
      /* Process message.  */
      if (((CHECK_FLAG (ilm->flags, NSM_MSG_ILM_ADD)) ||
           (CHECK_FLAG (ilm->flags, NSM_MSG_ILM_DELETE)) ||
           (CHECK_FLAG (ilm->flags, NSM_MSG_ILM_FAST_DELETE))) &&
          (CHECK_FLAG (ilm->flags, NSM_MSG_ILM_BIDIR)))
        {
          if (CHECK_FLAG (ilm->flags, NSM_MSG_ILM_ADD))
            {
              ret = nsm_gmpls_ilm_add_msg_process (nm, ilm, NSM_FALSE, iret,
                                                   NSM_FALSE);
              if (ret == NSM_SUCCESS || ret == NSM_ERR_ILM_INSTALL_FAILURE)
                {
                  if (ret == NSM_ERR_ILM_INSTALL_FAILURE)
                    {
                      zlog_warn (NSM_ZG, "ILM FEC: %r failed to add to the"
                                         " forwarder",
                                         &ilm->fec_prefix.u.prefix4);
                    }

                 if (CHECK_FLAG (ilm->flags, NSM_MSG_ILM_BIDIR))
                    {
                      SET_FLAG (iret->flags, NSM_MSG_REPLY_TO_BIDIR);
                    }

                  if (CHECK_FLAG (ilm->flags, NSM_MSG_GEN_ILM_FWD))
                    {
                      SET_FLAG (iret->flags, NSM_MSG_REPLY_TO_FWD);
                    }

                 iret->id = ilm->id;
                }
              else
                {
                  struct mpls_notification mn;

                  zlog_warn (NSM_ZG, "ILM add processing failed,"
                                     " err = %d", ret);
                  zlog_warn (NSM_ZG, "Sending notification");

                  /* Fill notification. */
                  pal_mem_set (&mn, 0, sizeof (struct mpls_notification));
                  pal_mem_cpy (&mn.owner, &ilm->owner,
                               sizeof (struct mpls_owner));
                  mn.id = ilm->id;
                  mn.type = MPLS_NTF_ILM_ADD_FAILURE;

                  ret = nsm_server_send_mpls_notification (header->vr_id,
                                                           header->vrf_id, nse,
                                                           header->message_id,
                                                           &mn);
                  return 0;
                }
            }
          else if (CHECK_FLAG (ilm->flags, NSM_MSG_ILM_FAST_DELETE))
            {
              /* Fast Delete */
              ret = nsm_gmpls_ilm_fast_del_msg_process (nm, ilm->iif_ix,
                                                        &ilm->in_label,
                                                        ilm->ilm_ix);
              if (ret != NSM_SUCCESS)
                zlog_warn (NSM_ZG, "ILM fast delete processing failed,"
                                   " err = %d", ret);
            }
          else
            {
              ret = nsm_gmpls_ilm_del_msg_process (nm, ilm);
              if (ret != NSM_SUCCESS)
                zlog_warn (NSM_ZG,
                             "ILM delete processing failed, err = %d", ret);

            }
        }
      else
        {
          struct mpls_notification mn;

          zlog_warn (NSM_ZG, "ILM add processing failed, err = %d", ret);
          zlog_warn (NSM_ZG, "Sending notification");

          /* Fill notification. */
          pal_mem_set (&mn, 0, sizeof (struct mpls_notification));
          pal_mem_cpy (&mn.owner, &ilm->owner, sizeof (struct mpls_owner));
          mn.id = ilm->id;
          mn.type = MPLS_NTF_ILM_ADD_FAILURE;

          ret = nsm_server_send_mpls_notification (header->vr_id,
                                                   header->vrf_id, nse,
                                                   header->message_id, &mn);
          return 0;
        }

      if (ilm == &iad->ilm_fwd)
        {
          ilm = &iad->ilm_bwd;
          ilm->rev_xc_ix = iret->xc_ix;
          ilm->rev_ilm_ix = iret->ilm_ix;
          iret = &ret_data.ilm_bwd;
        }
      else
        {
          ilm = NULL;
        }
    }

  nsm_server_send_ilm_bidir_reply (header->vr_id, header->vrf_id, nse,
                                   header->message_id, &ret_data);

  return NSM_SUCCESS;
}

#endif /* HAVE_GMPLS */

#ifdef HAVE_PACKET
#ifdef HAVE_DIFFSERV
/* Support DSCP has been updated.
   Send NSM_MSG_SUPPORTED_DSCP_UPDATE to client.*/
s_int32_t
nsm_server_send_supported_dscp_update (struct nsm_master *nm,
                                       struct nsm_server_entry *nse,
                                       u_char dscp, u_char support)
{
  struct nsm_msg_supported_dscp_update msg;
  s_int32_t len;
  vrf_id_t vrf_id = 0;

  msg.dscp = dscp;
  msg.support = support;

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service. */
  len = nsm_encode_supported_dscp_update (&nse->send.pnt, &nse->send.size, &msg);

  if (len < 0)
    return len;

  nsm_server_send_message (nse, nm->vr->id, vrf_id,
                           NSM_MSG_SUPPORTED_DSCP_UPDATE, 0, len);

  return 0;
}

/* dscp_exp map has been updated. Send NSM_MSG_DSCP_EXP_MAP_UPDATE to client.*/
s_int32_t
nsm_server_send_dscp_exp_map_update (struct nsm_master *nm,
                                     struct nsm_server_entry *nse,
                                     u_char exp, u_char dscp)
{
  struct nsm_msg_dscp_exp_map_update msg;
  s_int32_t len;
  vrf_id_t vrf_id = 0;

  msg.exp = exp;
  msg.dscp = dscp;

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service. */
  len = nsm_encode_dscp_exp_map_update (&nse->send.pnt, &nse->send.size, &msg);

  if (len < 0)
    return len;

  nsm_server_send_message (nse, nm->vr->id, vrf_id,
                           NSM_MSG_DSCP_EXP_MAP_UPDATE, 0, len);

  return 0;
}

/* Send supported dscp infomation to client. */
void
nsm_mpls_supported_dscp_update (struct nsm_master *nm,
                                u_char dscp, u_char support)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  s_int32_t i;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_DIFFSERV))
      {
        nsm_server_send_supported_dscp_update (nm, nse, dscp, support);

        if (IS_NSM_DEBUG_EVENT)
          zlog_info (NSM_ZG, "MESSAGE: NSM_MSG_SUPPORTED_DSCP_UPDATE");
      }
}


/* Send dscp_exp map update to client. */
void
nsm_mpls_dscp_exp_map_update (struct nsm_master *nm, u_char exp, u_char dscp)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  s_int32_t i;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_DIFFSERV))
      {
        nsm_server_send_dscp_exp_map_update (nm, nse, exp, dscp);

        if (IS_NSM_DEBUG_EVENT)
          zlog_info (NSM_ZG, "MESSAGE: NSM_MSG_DSCP_EXP_MAP_UPDATE");
      }
}

#endif /* HAVE_DIFFSERV */

#ifdef HAVE_DSTE
/* Send NSM_MSG_DSTE_TE_CLASS_UPDATE to client. */
s_int32_t
nsm_server_send_dste_te_class_update (struct nsm_master *nm,
                                      struct nsm_server_entry *nse,
                                      u_char te_class_index)
{
  struct nsm_msg_dste_te_class_update msg;
  vrf_id_t vrf_id = 0;
  s_int32_t len;

  msg.te_class_index = te_class_index;
  pal_mem_cpy (&msg.te_class, &NSM_MPLS->te_class[te_class_index],
               sizeof (struct te_class_s));
  if (msg.te_class.ct_num != CT_NUM_INVALID)
    pal_mem_cpy (msg.ct_name,
                 NSM_MPLS->ct_name[NSM_MPLS->te_class[te_class_index].ct_num],
                 MAX_CT_NAME_LEN + 1);
  else
    msg.ct_name[0] = '\0';

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service. */
  len = nsm_encode_dste_te_class_update (&nse->send.pnt, &nse->send.size, &msg);

  if (len < 0)
    return len;

  nsm_server_send_message (nse, nm->vr->id, vrf_id,
                           NSM_MSG_DSTE_TE_CLASS_UPDATE, 0, len);

  return 0;
}

/* Send TE CLASS update to client. */
void
nsm_dste_te_class_update (struct nsm_master *nm, u_char te_class_index)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  s_int32_t i;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_DSTE))
      {
        nsm_server_send_dste_te_class_update (nm, nse, te_class_index);

        if (IS_NSM_DEBUG_EVENT)
          zlog_info (NSM_ZG, "MESSAGE: NSM_MSG_DSTE_TE_CLASS_UPDATE");
      }
}

s_int32_t
nsm_server_send_bc_mode_update (struct nsm_server_entry *nse,
                                struct interface *ifp)
{
  struct nsm_msg_link msg;
  s_int32_t len;
  vrf_id_t vrf_id = 0;

  /* INTERFACE service is needed.  */
  if (! nsm_service_check (nse, NSM_SERVICE_INTERFACE))
    return 0;

  /* DSTE service is needed.  */
  if (! nsm_service_check (nse, NSM_SERVICE_DSTE))
    return 0;

  msg.ifindex = ifp->ifindex;
#ifdef HAVE_GMPLS
  msg.gifindex = ifp->gifindex;
#endif /* HAVE_GMPLS */
  msg.cindex = 0;
  NSM_SET_CTYPE (msg.cindex, NSM_LINK_CTYPE_BC_MODE);
  msg.bc_mode = ifp->bc_mode;

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM interface.  */
  len = nsm_encode_link (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  /* Send it to client.  */
  nsm_server_send_message (nse, ifp->vr->id, vrf_id,
                           NSM_MSG_LINK_ADD, 0, len);

  return 0;
}


/* Send TE CLASS update to client. */
void
nsm_dste_send_bc_mode_update (struct interface *ifp)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  s_int32_t i;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_DSTE))
      {
        nsm_server_send_bc_mode_update (nse, ifp);

        if (IS_NSM_DEBUG_EVENT)
          zlog_info (NSM_ZG, "MESSAGE: NSM_MSG_DSTE_INTERFACE_BC_MODE_UPDATE");
      }
}
#endif /* HAVE_DSTE */
#endif /* HAVE_PACKET */
#endif /* HAVE_MPLS */

#ifdef HAVE_MCAST_IPV4
s_int32_t
nsm_server_recv_ipv4_vif_add (struct nsm_msg_header *header,
                              void *arg, void *message)
{
  struct nsm_msg_ipv4_mrib_notification notif;
  struct nsm_msg_ipv4_vif_add *msg;
  struct nsm_server_entry *nse;
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_vrf *nvrf = NULL;
  u_int16_t vif_index;
  s_int32_t ret;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_ipv4_vif_add_dump (ns->zg, msg);

  /* Initialize notification message */
  pal_mem_set (&notif, 0, sizeof (notif));
  notif.type = NSM_MSG_IPV4_MRIB_NOTIFICATION_VIF_ADD;
  notif.key.vif_key.ifindex = msg->ifindex;
  notif.key.vif_key.flags = msg->flags;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (! nm)
    {
      notif.status = MRIB_INTERNAL_ERROR;
      ret = -1;
      goto exit;
    }

  nvrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (nvrf == NULL || nvrf->mcast == NULL)
    {
      notif.status = MRIB_INTERNAL_ERROR;
      ret = -1;
      goto exit;
    }

  /* Call vif add handler */
  ret = nsm_mcast_vif_add_msg_process (nvrf->mcast, msg, nse->nsc,
      &vif_index, &notif.key.vif_key.local_addr,
      &notif.key.vif_key.remote_addr);
  if (ret < 0)
    {
      /* send appropriate notification */
      switch (ret)
        {
        case NSM_ERR_MCAST_VIF_EXISTS:
          notif.status = MRIB_VIF_EXISTS;
          break;

        case NSM_ERR_MCAST_NO_VIF_INDEX:
          notif.status = MRIB_VIF_NO_INDEX;
          break;

        case NSM_ERR_MCAST_VIF_FWD_ADD_FAILURE:
          notif.status = MRIB_VIF_FWD_ADD_ERR;
          break;

        case NSM_ERR_MCAST_NO_MCAST:
          notif.status = MRIB_NO_MCAST;
          break;

        default:
          notif.status = MRIB_INTERNAL_ERROR;
          break;
        }
    }
  else
    {
      /* send vif add reply message */
      notif.status = MRIB_VIF_ADD_SUCCESS;
      notif.key.vif_key.vif_index = vif_index;
    }

exit:
  nsm_server_send_ipv4_mrib_notification (header->vr_id, header->vrf_id,
      nse, &notif, &header->message_id);

  /* Send IF Local-Membership Update */
  if (ret >= 0)
    nsm_mcast_vif_send_lmem_update (nvrf->mcast, msg);

  return 0;
}


s_int32_t
nsm_server_recv_ipv4_vif_del (struct nsm_msg_header *header,
                              void *arg, void *message)
{
  struct nsm_msg_ipv4_mrib_notification notif;
  struct nsm_msg_ipv4_vif_del *msg;
  struct nsm_server_entry *nse;
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_vrf *nvrf;
  u_int32_t ifindex;
  s_int32_t ret;

  /* Set connection and message.  */
  ifindex = 0;
  nse = arg;
  ns = nse->ns;
  msg = message;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_ipv4_vif_del_dump (ns->zg, msg);

  /* Initialize notification message */
  pal_mem_set (&notif, 0, sizeof (notif));
  notif.type = NSM_MSG_IPV4_MRIB_NOTIFICATION_VIF_DEL;
  notif.key.vif_key.vif_index = msg->vif_index;
  notif.key.vif_key.flags = msg->flags;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (! nm)
    {
      notif.status = MRIB_INTERNAL_ERROR;
      goto exit;
    }

  nvrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (nvrf == NULL || nvrf->mcast == NULL)
    {
      notif.status = MRIB_INTERNAL_ERROR;
      goto exit;
    }

  /* vif del handler */
  ret = nsm_mcast_vif_del_msg_process (nvrf->mcast, msg, nse->nsc, &ifindex);
  if (ret < 0)
    {
      /* handle delete processing failure here */
      switch (ret)
        {
        case NSM_ERR_MCAST_NO_VIF:
          notif.status = MRIB_VIF_NOT_EXIST;
          notif.key.vif_key.ifindex = ifindex;
          notif.key.vif_key.vif_index = msg->vif_index;
          break;

        case NSM_ERR_MCAST_VIF_NOT_BELONG:
          notif.status = MRIB_VIF_NOT_BELONG;
          notif.key.vif_key.ifindex = ifindex;
          notif.key.vif_key.vif_index = msg->vif_index;
          break;

        case NSM_ERR_MCAST_VIF_FWD_DEL_FAILURE:
          notif.status = MRIB_VIF_FWD_DEL_ERR;
          notif.key.vif_key.ifindex = ifindex;
          notif.key.vif_key.vif_index = msg->vif_index;
          break;

        case NSM_ERR_MCAST_NO_MCAST:
          notif.status = MRIB_NO_MCAST;
          notif.key.vif_key.ifindex = ifindex;
          notif.key.vif_key.vif_index = msg->vif_index;
          break;

        default:
          notif.status = MRIB_INTERNAL_ERROR;
          notif.key.vif_key.ifindex = ifindex;
          notif.key.vif_key.vif_index = msg->vif_index;
          break;
        }
    }
  else
    {
      /* send vif del reply message */
      notif.status = MRIB_VIF_DEL_SUCCESS;
      notif.key.vif_key.ifindex = ifindex;
      notif.key.vif_key.vif_index = msg->vif_index;
    }

exit:
  nsm_server_send_ipv4_mrib_notification (header->vr_id, header->vrf_id,
      nse, &notif, &header->message_id);

  return 0;
}

s_int32_t
nsm_server_recv_ipv4_mrt_add (struct nsm_msg_header *header,
                              void *arg, void *message)
{
  struct nsm_msg_ipv4_mrib_notification notif;
  struct nsm_msg_ipv4_mrt_add *msg;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_master *nm;
  struct nsm_vrf *nvrf;
  s_int32_t ret;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_ipv4_mrt_add_dump (ns->zg, msg);

  /* Initialize notification message */
  pal_mem_set (&notif, 0, sizeof (notif));
  notif.type = NSM_MSG_IPV4_MRIB_NOTIFICATION_MRT_ADD;
  notif.key.mrt_key.src = msg->src;
  notif.key.mrt_key.group = msg->group;
  notif.key.mrt_key.flags = msg->flags;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (! nm)
    {
      notif.status = MRIB_INTERNAL_ERROR;
      goto exit;
    }

  nvrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (nvrf == NULL || nvrf->mcast == NULL)
    {
      notif.status = MRIB_INTERNAL_ERROR;
      goto exit;
    }

  /* Mrt add handler */
  ret = nsm_mcast_mrt_add_msg_process (nvrf->mcast, msg, nse->nsc);
  if (ret < 0)
    {
      /* handle delete processing failure here */
      switch (ret)
        {
        case NSM_ERR_MCAST_MRT_EXISTS:
          notif.status = MRIB_MRT_EXISTS;
          break;

        case NSM_ERR_MCAST_MRT_LIMIT_EXCEED:
          notif.status = MRIB_MRT_LIMIT_EXCEEDED;
          break;

        case NSM_ERR_MCAST_MRT_FWD_ADD_FAILURE:
          notif.status = MRIB_MRT_FWD_ADD_ERR;
          break;

        case NSM_ERR_MCAST_NO_MCAST:
          notif.type = NSM_MSG_IPV4_MRIB_NOTIFICATION_NO_MCAST;
          break;

        default:
          notif.status = MRIB_INTERNAL_ERROR;
          break;
        }
    }
  else
    {
      /* send mrt add reply message */
      notif.status = MRIB_MRT_ADD_SUCCESS;
    }

exit:
  nsm_server_send_ipv4_mrib_notification (header->vr_id, header->vrf_id,
      nse, &notif, &header->message_id);


  return 0;
}

s_int32_t
nsm_server_recv_ipv4_mrt_del (struct nsm_msg_header *header,
                              void *arg, void *message)
{
  struct nsm_msg_ipv4_mrib_notification notif;
  struct nsm_msg_ipv4_mrt_del *msg;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_master *nm;
  struct nsm_vrf *nvrf;
  s_int32_t ret;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_ipv4_mrt_del_dump (ns->zg, msg);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (! nm)
    {
      notif.status = MRIB_INTERNAL_ERROR;
      goto exit;
    }

  nvrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (nvrf == NULL || nvrf->mcast == NULL)
    {
      notif.status = MRIB_INTERNAL_ERROR;
      goto exit;
    }

  /* Initialize notification message */
  pal_mem_set (&notif, 0, sizeof (notif));
  notif.type = NSM_MSG_IPV4_MRIB_NOTIFICATION_MRT_DEL;
  notif.key.mrt_key.src = msg->src;
  notif.key.mrt_key.group = msg->group;

  /* Mrt del handler */
  ret = nsm_mcast_mrt_del_msg_process (nvrf->mcast, msg, nse->nsc);
  if (ret < 0)
    {
      /* handle delete processing failure here */
      switch (ret)
        {
        case NSM_ERR_MCAST_NO_MRT:
          notif.status = MRIB_MRT_NOT_EXIST;
          break;

        case NSM_ERR_MCAST_MRT_NOT_BELONG:
          notif.status = MRIB_MRT_NOT_BELONG;
          break;

        case NSM_ERR_MCAST_MRT_FWD_DEL_FAILURE:
          notif.status = MRIB_MRT_FWD_DEL_ERR;
          break;

        case NSM_ERR_MCAST_NO_MCAST:
          notif.type = NSM_MSG_IPV4_MRIB_NOTIFICATION_NO_MCAST;
          break;

        default:
          notif.status = MRIB_INTERNAL_ERROR;
          break;
        }
    }
  else
    {
      /* send mrt del reply message */
      notif.status = MRIB_MRT_DEL_SUCCESS;
    }

exit:
  nsm_server_send_ipv4_mrib_notification (header->vr_id, header->vrf_id,
      nse, &notif, &header->message_id);

  return 0;
}

s_int32_t
nsm_server_recv_ipv4_mrt_stat_flags_update (struct nsm_msg_header *header,
                                            void *arg, void *message)
{
  struct nsm_msg_ipv4_mrt_stat_flags_update *msg;
  struct nsm_msg_ipv4_mrib_notification notif;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_master *nm;
  struct nsm_vrf *nvrf;
  s_int32_t ret;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_ipv4_mrt_stat_flags_update_dump (ns->zg, msg);

  pal_mem_set (&notif, 0, sizeof (notif));

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (! nm)
    {
      goto exit;
    }

  nvrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (nvrf == NULL || nvrf->mcast == NULL)
    {
      goto exit;
    }

  /* Mrt stat flags update handler */
  ret = nsm_mcast_mrt_stat_flags_update_msg_process (nvrf->mcast, msg, nse->nsc);
  if (ret == NSM_ERR_MCAST_NO_MCAST)
    {
      notif.type = NSM_MSG_IPV4_MRIB_NOTIFICATION_NO_MCAST;

      nsm_server_send_ipv4_mrib_notification (header->vr_id, header->vrf_id,
          nse, &notif, NULL);
    }

exit:
  return 0;
}

s_int32_t
nsm_server_recv_ipv4_mrt_state_refresh_flag_update (struct nsm_msg_header
                                                    *header, void *arg,
                                                    void *message)
{
  struct nsm_msg_ipv4_mrt_state_refresh_flag_update *msg;
  struct nsm_server_entry *nse;
  struct nsm_master *nm;
  struct nsm_server *ns;
  s_int32_t ret;

 /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
  return 0;

 /* Mrt state refresh flag update handler */
  ret = nsm_mcast_set_igmp_state_refresh_flag (nm, msg, nse->nsc);

  return 0;
}

s_int32_t
nsm_server_recv_ipv4_mrt_wholepkt_reply (struct nsm_msg_header *header,
                                         void *arg, void *message)
{
  struct nsm_msg_ipv4_mrt_wholepkt_reply *msg;
  struct nsm_msg_ipv4_mrib_notification notif;
  struct nsm_server_entry *nse;
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_vrf *nvrf;
  s_int32_t ret;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_ipv4_mrt_wholepkt_reply_dump (ns->zg, msg);

  pal_mem_set (&notif, 0, sizeof (notif));

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (! nm)
    {
      goto exit;
    }

  nvrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (nvrf == NULL || nvrf->mcast == NULL)
    {
      goto exit;
    }

  /* Mrt wholepkt reply handler here */
  ret = nsm_mcast_mrt_wholepkt_reply (nvrf->mcast, msg, nse->nsc);
  if (ret == NSM_ERR_MCAST_NO_MCAST)
    {
      notif.type = NSM_MSG_IPV4_MRIB_NOTIFICATION_NO_MCAST;

      nsm_server_send_ipv4_mrib_notification (header->vr_id, header->vrf_id,
          nse, &notif, NULL);
    }

exit:
  return 0;
}

s_int32_t
nsm_server_recv_mtrace_query_msg (struct nsm_msg_header *header,
                                  void *arg, void *message)
{
  struct nsm_msg_mtrace_query *msg;
  struct nsm_server_entry *nse;
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_vrf *nvrf;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_mtrace_query_dump_msg (ns->zg, msg);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (! nm)
    {
      goto exit;
    }

  nvrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (nvrf == NULL || nvrf->mcast == NULL)
    {
      goto exit;
    }

    /* Query msg handler */
  nsm_mtrace_recv_query_msg (nvrf->mcast, nse->nsc, msg);

exit:
  return 0;
}

s_int32_t
nsm_server_recv_mtrace_request_msg (struct nsm_msg_header *header,
                         void *arg, void *message)
{
  struct nsm_msg_mtrace_request *msg;
  struct nsm_server_entry *nse;
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_vrf *nvrf;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_mtrace_request_dump_msg (ns->zg, msg);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (! nm)
    {
      goto exit;
    }

  nvrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (nvrf == NULL || nvrf->mcast == NULL)
    {
      goto exit;
    }

  /* Request msg handler */
  nsm_mtrace_recv_request_msg (nvrf->mcast, nse->nsc, msg);

exit:
  return 0;
}

s_int32_t
nsm_server_send_ipv4_mrt_nocache (struct nsm_mcast *mcast,
                                  struct nsm_server_entry *nse,
                                  struct nsm_msg_ipv4_mrt_nocache *msg,
                                  u_int32_t *msg_id)
{
  s_int32_t len;

  /* Set encode pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_ipv4_mrt_nocache (&nse->send.pnt, &nse->send.size, msg);
  if (len < 0)
    return len;

  if (IS_NSM_DEBUG_SEND)
    {
      zlog_info (nse->ns->zg, "Sending NOCACHE to %s",
          modname_strl (nse->service.protocol_id));
      nsm_ipv4_mrt_nocache_dump (nse->ns->zg, msg);
    }

  /* Send it to client.  */
  nsm_server_send_message_msgid (nse,
                                 mcast->nvrf->nm->vr->id,
                                 mcast->nvrf->vrf->id,
                                 NSM_MSG_IPV4_MRT_NOCACHE,
                                 msg_id,
                                 len);

  return 0;
}


s_int32_t
nsm_server_send_ipv4_mrt_wrongvif (struct nsm_mcast *mcast,
                                   struct nsm_server_entry *nse,
                                   struct nsm_msg_ipv4_mrt_wrongvif *msg,
                                   u_int32_t *msg_id)
{
  s_int32_t len;

  /* Set encode pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_ipv4_mrt_wrongvif (&nse->send.pnt, &nse->send.size, msg);
  if (len < 0)
    return len;

  if (IS_NSM_DEBUG_SEND)
    {
      zlog_info (nse->ns->zg, "Sending WRONGVIF to %s",
          modname_strl (nse->service.protocol_id));
      nsm_ipv4_mrt_wrongvif_dump (nse->ns->zg, msg);
    }

  /* Send it to client.  */
  nsm_server_send_message_msgid (nse,
                                 mcast->nvrf->nm->vr->id,
                                 mcast->nvrf->vrf->id,
                                 NSM_MSG_IPV4_MRT_WRONGVIF,
                                 msg_id, len);

  return 0;
}


s_int32_t
nsm_server_send_ipv4_mrt_wholepkt_req (struct nsm_mcast *mcast,
                                       struct nsm_server_entry *nse,
                                       struct nsm_msg_ipv4_mrt_wholepkt_req *msg,
                                       u_int32_t *msg_id)
{
  s_int32_t len;

  /* Set encode pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_ipv4_mrt_wholepkt_req (&nse->send.pnt, &nse->send.size, msg);
  if (len < 0)
    return len;

  if (IS_NSM_DEBUG_SEND)
    {
      zlog_info (nse->ns->zg, "Sending WHOLEPKT REQ to %s",
          modname_strl (nse->service.protocol_id));
      nsm_ipv4_mrt_wholepkt_req_dump (nse->ns->zg, msg);
    }

  /* Send it to client.  */
  nsm_server_send_message_msgid (nse,
                                 mcast->nvrf->nm->vr->id,
                                 mcast->nvrf->vrf->id,
                                 NSM_MSG_IPV4_MRT_WHOLEPKT_REQ,
                                 msg_id, len);

  return 0;
}

s_int32_t
nsm_server_send_ipv4_mrt_stat_update (struct nsm_mcast *mcast,
                                      struct nsm_server_entry *nse,
                                      struct nsm_msg_ipv4_mrt_stat_update *msg,
                                      u_int32_t *msg_id)
{
  s_int32_t len;

  /* Set encode pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_ipv4_mrt_stat_update (&nse->send.pnt, &nse->send.size, msg);
  if (len < 0)
    return len;

  if (IS_NSM_DEBUG_SEND)
    {
      zlog_info (nse->ns->zg, "Sending MRT STAT UPDATE to %s",
          modname_strl (nse->service.protocol_id));
      nsm_ipv4_mrt_stat_update_dump (nse->ns->zg, msg);
    }

  /* Send it to client.  */
  nsm_server_send_message_msgid (nse,
                                 mcast->nvrf->nm->vr->id,
                                 mcast->nvrf->vrf->id,
                                 NSM_MSG_IPV4_MRT_STAT_UPDATE, msg_id, len);

  return 0;
}

s_int32_t
nsm_server_send_ipv4_mrib_notification (u_int32_t vr_id, vrf_id_t vrf_id,
                                        struct nsm_server_entry *nse,
                                        struct nsm_msg_ipv4_mrib_notification *msg,
                                        u_int32_t *msg_id)
{
  s_int32_t len;

  /* Set encode pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_ipv4_mrib_notification (&nse->send.pnt, &nse->send.size, msg);
  if (len < 0)
    return NSM_ERR_INTERNAL;

  if (IS_NSM_DEBUG_SEND)
    {
      zlog_info (nse->ns->zg, "Sending MRIB notification to %s",
          modname_strl (nse->service.protocol_id));
      nsm_ipv4_mrib_notification_dump (nse->ns->zg, msg);
    }

  /* Send it to client.  */
  nsm_server_send_message_msgid (nse,
                                 vr_id,
                                 vrf_id,
                                 NSM_MSG_IPV4_MRIB_NOTIFICATION,
                                 msg_id, len);

  return NSM_SUCCESS;
}

/* Send mtrace query to protocol */
s_int32_t
nsm_server_send_mtrace_query_msg (struct nsm_mcast *mcast,
                                  struct nsm_server_entry *nse,
                                  struct nsm_msg_mtrace_query *msg)
{
  s_int32_t len;

  /* Set encode pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  len = nsm_encode_mtrace_query_msg (&nse->send.pnt, &nse->send.size, msg);
  if (len < 0)
    return len;

  if (IS_NSM_DEBUG_SEND)
    {
      zlog_info (nse->ns->zg, "Sending Mtrace query msg to %s",
          modname_strl (nse->service.protocol_id));
      nsm_mtrace_query_dump_msg (nse->ns->zg, msg);
    }

  /* Send it to client.  */
  return nsm_server_send_message (nse,
                                  mcast->nvrf->nm->vr->id,
                                  mcast->nvrf->vrf->id,
                                  NSM_MSG_MTRACE_QUERY, 0, len);
}

/* Send mtrace request to protocol */
s_int32_t
nsm_server_send_mtrace_request_msg (struct nsm_mcast *mcast,
                                    struct nsm_server_entry *nse,
                                    struct nsm_msg_mtrace_request *msg)
{
  s_int32_t len;

  /* Set encode pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  len = nsm_encode_mtrace_request_msg (&nse->send.pnt, &nse->send.size, msg);
  if (len < 0)
    return len;

  if (IS_NSM_DEBUG_SEND)
    {
      zlog_info (nse->ns->zg, "Sending Mtrace request msg to %s",
          modname_strl (nse->service.protocol_id));
      nsm_mtrace_request_dump_msg (nse->ns->zg, msg);
    }

  /* Send it to client.  */
  return nsm_server_send_message (nse,
                                  mcast->nvrf->nm->vr->id,
                                  mcast->nvrf->vrf->id,
                                  NSM_MSG_MTRACE_REQUEST, 0, len);
}

/* Send IGMP Local-Membership message to client. */
s_int32_t
nsm_server_send_igmp (struct nsm_mcast *mcast,
                      struct nsm_server_entry *nse,
                      struct nsm_msg_igmp *msg)
{
  s_int32_t len;

  /* Set encode pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_igmp (&nse->send.pnt, &nse->send.size, msg);
  if (len < 0)
    return len;

  if (IS_NSM_DEBUG_SEND)
    {
      zlog_info (nse->ns->zg, "Sending IGMP Local membership update to %s",
                 modname_strl (nse->service.protocol_id));

      nsm_igmp_dump (nse->ns->zg, msg);
    }

  /* Send it to client.  */
  nsm_server_send_message (nse,
                           mcast->nvrf->nm->vr->id,
                           mcast->nvrf->vrf->id,
                           NSM_MSG_IGMP_LMEM, 0, len);

  return 0;
}

#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
s_int32_t
nsm_server_recv_ipv6_mif_add (struct nsm_msg_header *header,
                              void *arg, void *message)
{
  struct nsm_msg_ipv6_mrib_notification notif;
  struct nsm_msg_ipv6_mif_add *msg;
  struct nsm_server_entry *nse;
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_vrf *nvrf = NULL;
  u_int16_t mif_index;
  s_int32_t ret;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;
  nvrf = NULL;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_ipv6_mif_add_dump (ns->zg, msg);

  /* Initialize notification message */
  pal_mem_set (&notif, 0, sizeof (notif));
  notif.type = NSM_MSG_IPV6_MRIB_NOTIFICATION_MIF_ADD;
  notif.key.mif_key.ifindex = msg->ifindex;
  notif.key.mif_key.flags = msg->flags;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (! nm)
    {
      notif.status = MRIB_INTERNAL_ERROR;
      ret = -1;
      goto exit;
    }

  nvrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (nvrf == NULL || nvrf->mcast6 == NULL)
    {
      notif.status = MRIB_INTERNAL_ERROR;
      ret = -1;
      goto exit;
    }

  /* Call mif add handler */
  ret = nsm_mcast6_mif_add_msg_process (nvrf->mcast6, msg, nse->nsc, &mif_index);
  if (ret < 0)
    {
      /* send appropriate notification */
      switch (ret)
        {
        case NSM_ERR_MCAST6_MIF_EXISTS:
          notif.status = MRIB_MIF_EXISTS;
          break;

        case NSM_ERR_MCAST6_NO_MIF_INDEX:
          notif.status = MRIB_MIF_NO_INDEX;
          break;

        case NSM_ERR_MCAST6_MIF_FWD_ADD_FAILURE:
          notif.status = MRIB_MIF_FWD_ADD_ERR;
          break;

        case NSM_ERR_MCAST6_NO_MCAST:
          notif.status = MRIB_NO_MCAST;
          break;

        default:
          notif.status = MRIB_INTERNAL_ERROR;
          break;
        }
    }
  else
    {
      /* send mif add reply message */
      notif.status = MRIB_MIF_ADD_SUCCESS;
      notif.key.mif_key.mif_index = mif_index;
    }

exit:
  nsm_server_send_ipv6_mrib_notification (header->vr_id, header->vrf_id,
      nse, &notif, &header->message_id);

  /* Send IF Local-Membership Update */
  if (nvrf != NULL && ret >= 0)
    nsm_mcast6_mif_send_lmem_update (nvrf->mcast6, msg);

  return 0;
}


s_int32_t
nsm_server_recv_ipv6_mif_del (struct nsm_msg_header *header,
                              void *arg, void *message)
{
  struct nsm_msg_ipv6_mrib_notification notif;
  struct nsm_msg_ipv6_mif_del *msg;
  struct nsm_server_entry *nse;
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_vrf *nvrf;
  u_int32_t ifindex;
  s_int32_t ret;

  /* Set connection and message.  */
  ifindex = 0;
  nse = arg;
  ns = nse->ns;
  msg = message;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_ipv6_mif_del_dump (ns->zg, msg);

  /* Initialize notification message */
  pal_mem_set (&notif, 0, sizeof (notif));
  notif.type = NSM_MSG_IPV6_MRIB_NOTIFICATION_MIF_DEL;
  notif.key.mif_key.mif_index = msg->mif_index;
  notif.key.mif_key.flags = msg->flags;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (! nm)
    {
      notif.status = MRIB_INTERNAL_ERROR;
      goto exit;
    }

  nvrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (nvrf == NULL || nvrf->mcast6 == NULL)
    {
      notif.status = MRIB_INTERNAL_ERROR;
      goto exit;
    }

  /* mif del handler */
  ret = nsm_mcast6_mif_del_msg_process (nvrf->mcast6, msg, nse->nsc, &ifindex);
  if (ret < 0)
    {
      /* handle delete processing failure here */
      switch (ret)
        {
        case NSM_ERR_MCAST6_NO_MIF:
          notif.status = MRIB_MIF_NOT_EXIST;
          notif.key.mif_key.ifindex = ifindex;
          notif.key.mif_key.mif_index = msg->mif_index;
          break;

        case NSM_ERR_MCAST6_MIF_NOT_BELONG:
          notif.status = MRIB_MIF_NOT_BELONG;
          notif.key.mif_key.ifindex = ifindex;
          notif.key.mif_key.mif_index = msg->mif_index;
          break;

        case NSM_ERR_MCAST6_MIF_FWD_DEL_FAILURE:
          notif.status = MRIB_MIF_FWD_DEL_ERR;
          notif.key.mif_key.ifindex = ifindex;
          notif.key.mif_key.mif_index = msg->mif_index;
          break;

        case NSM_ERR_MCAST6_NO_MCAST:
          notif.status = MRIB_NO_MCAST;
          notif.key.mif_key.ifindex = ifindex;
          notif.key.mif_key.mif_index = msg->mif_index;
          break;

        default:
          notif.status = MRIB_INTERNAL_ERROR;
          notif.key.mif_key.ifindex = ifindex;
          notif.key.mif_key.mif_index = msg->mif_index;
          break;
        }
    }
  else
    {
      /* send mif del reply message */
      notif.status = MRIB_MIF_DEL_SUCCESS;
      notif.key.mif_key.ifindex = ifindex;
      notif.key.mif_key.mif_index = msg->mif_index;
    }

exit:
  nsm_server_send_ipv6_mrib_notification (header->vr_id, header->vrf_id,
      nse, &notif, &header->message_id);

  return 0;
}

s_int32_t
nsm_server_recv_ipv6_mrt_add (struct nsm_msg_header *header,
                              void *arg, void *message)
{
  struct nsm_msg_ipv6_mrib_notification notif;
  struct nsm_msg_ipv6_mrt_add *msg;
  struct nsm_server_entry *nse;
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_vrf *nvrf;
  s_int32_t ret;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_ipv6_mrt_add_dump (ns->zg, msg);

  /* Initialize notification message */
  pal_mem_set (&notif, 0, sizeof (notif));
  notif.type = NSM_MSG_IPV6_MRIB_NOTIFICATION_MRT_ADD;
  notif.key.mrt_key.src = msg->src;
  notif.key.mrt_key.group = msg->group;
  notif.key.mrt_key.flags = msg->flags;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (! nm)
    {
      notif.status = MRIB_INTERNAL_ERROR;
      goto exit;
    }

  nvrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (nvrf == NULL || nvrf->mcast6 == NULL)
    {
      notif.status = MRIB_INTERNAL_ERROR;
      goto exit;
    }

  /* Mrt add handler */
  ret = nsm_mcast6_mrt_add_msg_process (nvrf->mcast6, msg, nse->nsc);
  if (ret < 0)
    {
      /* handle delete processing failure here */
      switch (ret)
        {
        case NSM_ERR_MCAST6_MRT_EXISTS:
          notif.status = MRIB_MRT_EXISTS;
          break;

        case NSM_ERR_MCAST6_MRT_LIMIT_EXCEED:
          notif.status = MRIB_MRT_LIMIT_EXCEEDED;
          break;

        case NSM_ERR_MCAST6_MRT_FWD_ADD_FAILURE:
          notif.status = MRIB_MRT_FWD_ADD_ERR;
          break;

        case NSM_ERR_MCAST6_NO_MCAST:
          notif.type = NSM_MSG_IPV6_MRIB_NOTIFICATION_NO_MCAST;
          break;

        default:
          notif.status = MRIB_INTERNAL_ERROR;
          break;
        }
    }
  else
    {
      /* send mrt add reply message */
      notif.status = MRIB_MRT_ADD_SUCCESS;
    }

exit:
  nsm_server_send_ipv6_mrib_notification (header->vr_id, header->vrf_id,
      nse, &notif, &header->message_id);

  return 0;
}

s_int32_t
nsm_server_recv_ipv6_mrt_del (struct nsm_msg_header *header,
                              void *arg, void *message)
{
  struct nsm_msg_ipv6_mrib_notification notif;
  struct nsm_msg_ipv6_mrt_del *msg;
  struct nsm_server_entry *nse;
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_vrf *nvrf;
  s_int32_t ret;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_ipv6_mrt_del_dump (ns->zg, msg);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (! nm)
    {
      notif.status = MRIB_INTERNAL_ERROR;
      goto exit;
    }

  nvrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (nvrf == NULL || nvrf->mcast6 == NULL)
    {
      notif.status = MRIB_INTERNAL_ERROR;
      goto exit;
    }

  /* Initialize notification message */
  pal_mem_set (&notif, 0, sizeof (notif));
  notif.type = NSM_MSG_IPV6_MRIB_NOTIFICATION_MRT_DEL;
  notif.key.mrt_key.src = msg->src;
  notif.key.mrt_key.group = msg->group;

  /* Mrt del handler */
  ret = nsm_mcast6_mrt_del_msg_process (nvrf->mcast6, msg, nse->nsc);
  if (ret < 0)
    {
      /* handle delete processing failure here */
      switch (ret)
        {
        case NSM_ERR_MCAST6_NO_MRT:
          notif.status = MRIB_MRT_NOT_EXIST;
          break;

        case NSM_ERR_MCAST6_MRT_NOT_BELONG:
          notif.status = MRIB_MRT_NOT_BELONG;
          break;

        case NSM_ERR_MCAST6_MRT_FWD_DEL_FAILURE:
          notif.status = MRIB_MRT_FWD_DEL_ERR;
          break;

        case NSM_ERR_MCAST6_NO_MCAST:
          notif.type = NSM_MSG_IPV6_MRIB_NOTIFICATION_NO_MCAST;
          break;

        default:
          notif.status = MRIB_INTERNAL_ERROR;
          break;
        }
    }
  else
    {
      /* send mrt del reply message */
      notif.status = MRIB_MRT_DEL_SUCCESS;
    }

exit:
  nsm_server_send_ipv6_mrib_notification (header->vr_id, header->vrf_id,
      nse, &notif, &header->message_id);

  return 0;
}

s_int32_t
nsm_server_recv_ipv6_mrt_stat_flags_update (struct nsm_msg_header *header,
                                            void *arg, void *message)
{
  struct nsm_msg_ipv6_mrib_notification notif;
  struct nsm_msg_ipv6_mrt_stat_flags_update *msg;
  struct nsm_server_entry *nse;
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_vrf *nvrf;
  s_int32_t ret;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_ipv6_mrt_stat_flags_update_dump (ns->zg, msg);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (! nm)
    {
      goto exit;
    }

  nvrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (nvrf == NULL || nvrf->mcast6 == NULL)
    {
      goto exit;
    }

  /* Mrt stat flags update handler */
  ret = nsm_mcast6_mrt_stat_flags_update_msg_process (nvrf->mcast6,
                                                      msg, nse->nsc);
  if (ret == NSM_ERR_MCAST6_NO_MCAST)
    {
      pal_mem_set (&notif, 0, sizeof (notif));
      notif.type = NSM_MSG_IPV6_MRIB_NOTIFICATION_NO_MCAST;

      nsm_server_send_ipv6_mrib_notification (header->vr_id, header->vrf_id,
          nse, &notif, NULL);
    }

exit:
  return 0;
}

s_int32_t
nsm_server_recv_ipv6_mrt_state_refresh_flag_update (struct nsm_msg_header *header,
                                                    void *arg,
                                                    void *message)
{
  struct nsm_msg_ipv6_mrt_state_refresh_flag_update *msg;
  struct nsm_server_entry *nse;
  struct nsm_master *nm;
  struct nsm_server *ns;
  s_int32_t ret;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
  return 0;

  /* Mrt state refresh flag update handler */
  ret = nsm_mcast6_set_mld_state_refresh_flag (nm, msg, nse->nsc);

  return 0;
}

s_int32_t

nsm_server_recv_ipv6_mrt_wholepkt_reply (struct nsm_msg_header *header,
                                         void *arg, void *message)
{
  struct nsm_msg_ipv6_mrt_wholepkt_reply *msg;
  struct nsm_msg_ipv6_mrib_notification notif;
  struct nsm_server_entry *nse;
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_vrf *nvrf;
  s_int32_t ret;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_ipv6_mrt_wholepkt_reply_dump (ns->zg, msg);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (! nm)
    {
      notif.status = MRIB_INTERNAL_ERROR;
      goto exit;
    }

  nvrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (nvrf == NULL || nvrf->mcast6 == NULL)
    {
      notif.status = MRIB_INTERNAL_ERROR;
      goto exit;
    }

  /* Mrt wholepkt reply handler here */
  ret = nsm_mcast6_mrt_wholepkt_reply (nvrf->mcast6, msg, nse->nsc);
  if (ret == NSM_ERR_MCAST6_NO_MCAST)
    {
      pal_mem_set (&notif, 0, sizeof (notif));
      notif.type = NSM_MSG_IPV6_MRIB_NOTIFICATION_NO_MCAST;

      nsm_server_send_ipv6_mrib_notification (header->vr_id, header->vrf_id,
          nse, &notif, NULL);
    }

exit:
  return 0;
}

/* Send MLD Local-Membership message to client. */
s_int32_t
nsm_server_send_mld (struct nsm_mcast6 *mcast6,
                     struct nsm_server_entry *nse,
                     struct nsm_msg_mld *msg)
{
  s_int32_t len;

  /* Set encode pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_mld (&nse->send.pnt, &nse->send.size, msg);
  if (len < 0)
    return len;

  if (IS_NSM_DEBUG_SEND)
    {
      zlog_info (nse->ns->zg, "Sending MLD Local membership update to %s",
                 modname_strl (nse->service.protocol_id));

      nsm_mld_dump (nse->ns->zg, msg);
    }

  /* Send it to client.  */
  nsm_server_send_message (nse,
                           mcast6->nvrf->nm->vr->id,
                           mcast6->nvrf->vrf->id,
                           NSM_MSG_MLD_LMEM, 0, len);

  return 0;
}

s_int32_t
nsm_server_send_ipv6_mrt_nocache (struct nsm_mcast6 *mcast6,
                                  struct nsm_server_entry *nse,
                                  struct nsm_msg_ipv6_mrt_nocache *msg,
                                  u_int32_t *msg_id)
{
  s_int32_t len;

  /* Set encode pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_ipv6_mrt_nocache (&nse->send.pnt, &nse->send.size, msg);
  if (len < 0)
    return len;

  if (IS_NSM_DEBUG_SEND)
    {
      zlog_info (nse->ns->zg, "Sending NOCACHE to %s",
          modname_strl (nse->service.protocol_id));
      nsm_ipv6_mrt_nocache_dump (nse->ns->zg, msg);
    }

  /* Send it to client.  */
  nsm_server_send_message_msgid (nse,
                                 mcast6->nvrf->nm->vr->id,
                                 mcast6->nvrf->vrf->id,
                                 NSM_MSG_IPV6_MRT_NOCACHE, msg_id, len);

  return 0;
}


s_int32_t
nsm_server_send_ipv6_mrt_wrongmif (struct nsm_mcast6 *mcast6,
                                   struct nsm_server_entry *nse,
                                   struct nsm_msg_ipv6_mrt_wrongmif *msg,
                                   u_int32_t *msg_id)
{
  s_int32_t len;

  /* Set encode pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_ipv6_mrt_wrongmif (&nse->send.pnt, &nse->send.size, msg);
  if (len < 0)
    return len;

  if (IS_NSM_DEBUG_SEND)
    {
      zlog_info (nse->ns->zg, "Sending WRONGMIF to %s",
          modname_strl (nse->service.protocol_id));
      nsm_ipv6_mrt_wrongmif_dump (nse->ns->zg, msg);
    }

  /* Send it to client.  */
  nsm_server_send_message_msgid (nse,
                                 mcast6->nvrf->nm->vr->id,
                                 mcast6->nvrf->vrf->id,
                                 NSM_MSG_IPV6_MRT_WRONGMIF, msg_id, len);

  return 0;
}


s_int32_t
nsm_server_send_ipv6_mrt_wholepkt_req (struct nsm_mcast6 *mcast6,
                                       struct nsm_server_entry *nse,
                                       struct nsm_msg_ipv6_mrt_wholepkt_req *msg,
                                       u_int32_t *msg_id)
{
  s_int32_t len;

  /* Set encode pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_ipv6_mrt_wholepkt_req (&nse->send.pnt, &nse->send.size, msg);
  if (len < 0)
    return len;

  if (IS_NSM_DEBUG_SEND)
    {
      zlog_info (nse->ns->zg, "Sending WHOLEPKT REQ to %s",
          modname_strl (nse->service.protocol_id));
      nsm_ipv6_mrt_wholepkt_req_dump (nse->ns->zg, msg);
    }

  /* Send it to client.  */
  nsm_server_send_message_msgid (nse,
                                 mcast6->nvrf->nm->vr->id,
                                 mcast6->nvrf->vrf->id,
                                 NSM_MSG_IPV6_MRT_WHOLEPKT_REQ, msg_id, len);

  return 0;
}

s_int32_t
nsm_server_send_ipv6_mrt_stat_update (struct nsm_mcast6 *mcast6,
                                      struct nsm_server_entry *nse,
                                      struct nsm_msg_ipv6_mrt_stat_update *msg,
                                      u_int32_t *msg_id)
{
  s_int32_t len;

  /* Set encode pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_ipv6_mrt_stat_update (&nse->send.pnt, &nse->send.size, msg);
  if (len < 0)
    return len;

  if (IS_NSM_DEBUG_SEND)
    {
      zlog_info (nse->ns->zg, "Sending MRT STAT UPDATE to %s",
          modname_strl (nse->service.protocol_id));
      nsm_ipv6_mrt_stat_update_dump (nse->ns->zg, msg);
    }

  /* Send it to client.  */
  nsm_server_send_message_msgid (nse,
                                 mcast6->nvrf->nm->vr->id,
                                 mcast6->nvrf->vrf->id,
                                 NSM_MSG_IPV6_MRT_STAT_UPDATE, msg_id, len);

  return 0;
}

s_int32_t
nsm_server_send_ipv6_mrib_notification (u_int32_t vr_id, vrf_id_t vrf_id,
                                        struct nsm_server_entry *nse,
                                        struct nsm_msg_ipv6_mrib_notification *msg,
                                        u_int32_t *msg_id)
{
  s_int32_t len;

  /* Set encode pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service.  */
  len = nsm_encode_ipv6_mrib_notification (&nse->send.pnt, &nse->send.size, msg);
  if (len < 0)
    return len;

  if (IS_NSM_DEBUG_SEND)
    {
      zlog_info (nse->ns->zg, "Sending MRIB notification to %s",
          modname_strl (nse->service.protocol_id));
      nsm_ipv6_mrib_notification_dump (nse->ns->zg, msg);
    }

  /* Send it to client.  */
  nsm_server_send_message_msgid (nse,
                                 vr_id,
                                 vrf_id,
                                 NSM_MSG_IPV6_MRIB_NOTIFICATION, msg_id, len);

  return 0;
}
#endif /* HAVE_MCAST_IPV6 */

#ifdef HAVE_GMPLS
s_int32_t
nsm_server_send_gmplsif_flush (u_int32_t vr_id, vrf_id_t vrf_id,
                               struct nsm_server_bulk_buf *buf,
                               struct nsm_server_entry *nse,
                               s_int32_t msg_type)
{
  s_int32_t nbytes;
  struct nsm_server_entry_buf *temp_buf = NULL;

  if (! buf || ! nse)
    return 0;

  temp_buf = &(buf->bulk_buf);
  buf->bulk_thread = NULL;

  /* Buffer is empty */
  if (temp_buf->pnt == NULL)
    return 0;

  nbytes = temp_buf->pnt - temp_buf->buf - NSM_MSG_HEADER_SIZE;

  pal_mem_cpy (&nse->send, temp_buf, sizeof (struct nsm_server_entry_buf));

  nsm_server_send_message (nse, vr_id, vrf_id, msg_type, 0, nbytes);

  temp_buf->pnt = NULL;
  temp_buf->len = NSM_MESSAGE_MAX_LEN;
  pal_mem_set (temp_buf->buf, 0, NSM_MESSAGE_MAX_LEN);
  return 0;
}

void
nsm_server_data_link_msg_set (struct datalink *dl,
                              cindex_t cindex,
                              struct nsm_msg_data_link *msg)
{
  pal_mem_set (msg, '\0', sizeof (struct nsm_msg_data_link));

  msg->cindex = cindex;
  pal_mem_cpy (msg->name, dl->name, GINTERFACE_NAMSIZ);
  msg->gifindex = dl->gifindex;

  if (dl->ifp)
    msg->ifindex = dl->ifp->ifindex;

  GMPLS_LINK_ID_COPY (&msg->l_linkid, &dl->l_linkid);
  GMPLS_LINK_ID_COPY (&msg->r_linkid, &dl->r_linkid);

  msg->status = dl->status;
  msg->flags = dl->flags;

  if (dl->prop)
    {
      msg->prop = *dl->prop;
    }

  if (dl->telink)
    {
      msg->te_gifindex = dl->telink->gifindex;
      pal_mem_cpy (msg->tlname, dl->telink->name, GINTERFACE_NAMSIZ);
    }
   if (dl->ifp)
     msg->p_prop = dl->ifp->phy_prop;

  return;
}

void
nsm_server_data_link_sub_msg_set (struct datalink *dl,
                                  cindex_t cindex,
                                  struct nsm_msg_data_link_sub *msg)
{
  pal_mem_set (msg, '\0', sizeof (struct nsm_msg_data_link_sub));

  msg->cindex = cindex;
  pal_mem_cpy (msg->name, dl->name, GINTERFACE_NAMSIZ);
  msg->gifindex = dl->gifindex;

  if (dl->ifp)
    msg->ifindex = dl->ifp->ifindex;

  GMPLS_LINK_ID_COPY (&msg->l_linkid, &dl->l_linkid);
  GMPLS_LINK_ID_COPY (&msg->r_linkid, &dl->r_linkid);

  msg->status = dl->status;
  msg->flags = dl->flags;
  if (dl->telink)
    {
      msg->te_gifindex = dl->telink->gifindex;
      pal_mem_cpy (msg->tlname, dl->telink->name, GINTERFACE_NAMSIZ);
    }

  return;
}

void
nsm_server_te_link_msg_set (struct telink *tl,
                            cindex_t cindex,
                            struct nsm_msg_te_link *msg)
{
  u_char ii;

  pal_mem_set (msg, '\0', sizeof (struct nsm_msg_te_link));

  msg->cindex = cindex;
  pal_strcpy (msg->name, tl->name);
  msg->gifindex = tl->gifindex;

  GMPLS_LINK_ID_COPY (&msg->l_linkid, &tl->l_linkid);
  GMPLS_LINK_ID_COPY (&msg->r_linkid, &tl->r_linkid);
  msg->addr = tl->addr;
  msg->remote_addr = tl->remote_addr;
  msg->status = tl->status;

  msg->l_prop = tl->prop.lpro;
  msg->p_prop = tl->prop.phy_prop;
  msg->admin_group = tl->prop.admin_group;
  msg->linkid = tl->prop.linkid;
  msg->flags = tl->flags;
  msg->mtu = tl->prop.mtu;

  /* SRLG encoding decoding still needs to be looked at */
  msg->srlg_num = 0;

  if (tl->prop.srlg->max)
    {
      msg->srlg = XCALLOC (MTYPE_GMPLS_SRLG, sizeof(u_int32_t)*tl->prop.srlg->max);
      if (!msg->srlg)
        return;
      for (ii = 0; ii < tl->prop.srlg->max; ii++)
        {
          if (tl->prop.srlg->index[ii] != NULL)
            {
              msg->srlg[msg->srlg_num] = (u_int32_t) *((u_int32_t*)
                                                 tl->prop.srlg->index[ii]);
              msg->srlg_num++;
              if (msg->srlg_num >= MAX_NUM_SRLG_SUPPORTED)
                {
                  break;
                }
            }
        }
    }

  if (tl->control_adj)
    {
      msg->ca_gifindex = tl->control_adj->gifindex;
      pal_mem_cpy (msg->caname, tl->control_adj->name, GINTERFACE_NAMSIZ);
    }

  return;
}

void
nsm_server_control_channel_msg_set (struct control_channel *cc,
                                   cindex_t cindex,
                                   struct nsm_msg_control_channel *msg)
{
  msg->cindex = cindex;
  pal_mem_cpy (msg->name, cc->name, GINTERFACE_NAMSIZ);
  msg->gifindex = cc->gifindex;
  msg->bifindex = cc->bifindex;
  msg->ccid = cc->ccid;
  pal_mem_cpy (&msg->l_addr, &cc->l_addr, sizeof (struct pal_in4_addr));
  pal_mem_cpy (&msg->r_addr, &cc->r_addr, sizeof (struct pal_in4_addr));
  msg->status = cc->status;
  msg->primary = cc->primary;
  if (cc->ca)
    {
      msg->cagifindex = cc->ca->gifindex;
      pal_mem_cpy (msg->caname, cc->ca->name, GINTERFACE_NAMSIZ);
    }
  else
    msg->cagifindex = 0;
}

s_int32_t
nsm_server_send_te_link (u_int32_t vr_id, vrf_id_t vrf_id,
                         struct nsm_server_entry *nse,
                         struct telink *tl,
                         cindex_t cindex)
{
  struct apn_vr *vr;
  struct apn_vrf *iv;
  struct nsm_msg_te_link msg;
  s_int32_t len;

  if (! tl || ! tl->gmif || ! tl->gmif->vr || ! tl->gmif->vrf)
    return 0;

  vr = tl->gmif->vr;
  iv = tl->gmif->vrf;

  /* Check requested service */
  if (! nsm_service_check (nse, NSM_SERVICE_TE_LINK))
    return 0;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_te_link));
  nsm_server_te_link_msg_set (tl, cindex, &msg);

  /* Set encode pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  if (IS_NSM_DEBUG_SEND)
    {
      zlog_info (nse->ns->zg, "NSM SEND TE Link");
      nsm_te_link_dump (nse->ns->zg, &msg);
    }

  /* Encode NSM service. */
  len = nsm_encode_te_link (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    {
      if (msg.srlg)
        XFREE (MTYPE_GMPLS_SRLG, msg.srlg);

      return len;
    }

  /* Send it to client */
  nsm_server_send_message (nse, vr->id, iv->id,
                           NSM_MSG_TE_LINK, 0, len);

  if (msg.srlg)
    XFREE (MTYPE_GMPLS_SRLG, msg.srlg);
  return 0;
}

s_int32_t
nsm_server_send_data_link (u_int32_t vr_id, vrf_id_t vrf_id,
                           struct nsm_server_entry *nse,
                           struct datalink *dl,
                           cindex_t cindex)
{
  struct apn_vr *vr;
  struct apn_vrf *iv;
  struct nsm_msg_data_link msg;
  s_int32_t len;

  if (! dl || ! dl->gmif || ! dl->gmif->vr || ! dl->gmif->vrf)
    return 0;

  vr = dl->gmif->vr;
  iv = dl->gmif->vrf;

  /* Check requested service */
  if (! nsm_service_check (nse, NSM_SERVICE_DATA_LINK))
    return 0;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_data_link));
  nsm_server_data_link_msg_set (dl, cindex, &msg);

  /* Set encode pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service. */
  len = nsm_encode_data_link (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  /* Send it to client */
  nsm_server_send_message (nse, vr->id, iv->id,
                           NSM_MSG_DATA_LINK, 0, len);
  return 0;
}

s_int32_t
nsm_server_send_data_link_sub (u_int32_t vr_id, vrf_id_t vrf_id,
                               struct nsm_server_entry *nse,
                               struct datalink *dl,
                               cindex_t cindex)
{
  struct apn_vr *vr;
  struct apn_vrf *iv;
  struct nsm_msg_data_link_sub msg;
  s_int32_t len;

  if (! dl || ! dl->gmif || ! dl->gmif->vr || ! dl->gmif->vrf)
    return 0;

  vr = dl->gmif->vr;
  iv = dl->gmif->vrf;

  /* Check requested service */
  if (! nsm_service_check (nse, NSM_SERVICE_DATA_LINK_SUB))
    return 0;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_data_link_sub));
  nsm_server_data_link_sub_msg_set (dl, cindex, &msg);

  /* Set encode pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM service. */
  len = nsm_encode_data_link_sub (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  /* Send it to client */
  nsm_server_send_message (nse, vr->id, iv->id,
                           NSM_MSG_DATA_LINK_SUB, 0, len);
  return 0;
}


s_int32_t
nsm_server_send_gmpls_control_channel (u_int32_t vr_id, vrf_id_t vrf_id,
                                       struct nsm_server_entry *nse,
                                       struct control_channel *cc,
                                       cindex_t cindex)
{
  struct apn_vr *vr;
  struct apn_vrf *iv;
  struct nsm_msg_control_channel msg;
  s_int32_t len;

  if (! cc || ! cc->gmif || ! cc->gmif->vr || ! cc->gmif->vrf)
    return 0;

  vr = cc->gmif->vr;
  iv = cc->gmif->vrf;

  /* Check requested service */
  if (! nsm_service_check (nse, NSM_SERVICE_CONTROL_CHANNEL))
    return 0;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_control_channel));
  nsm_server_control_channel_msg_set (cc, cindex, &msg);

  /* Set encode pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  if (IS_NSM_DEBUG_SEND)
    {
      zlog_info (nse->ns->zg, "NSM SEND Control Channel");
      nsm_control_channel_dump (nse->ns->zg, &msg);
    }

  /* Encode NSM service. */
  len = nsm_encode_control_channel (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  /* Send it to client */
  nsm_server_send_message (nse, vr->id, iv->id,
                           NSM_MSG_CONTROL_CHANNEL, 0, len);
  return 0;
}


s_int32_t
nsm_server_send_gmpls_control_adj (u_int32_t vr_id, vrf_id_t vrf_id,
                                   struct nsm_server_entry *nse,
                                   struct control_adj *ca,
                                   cindex_t cindex)
{
  struct apn_vr *vr;
  struct apn_vrf *iv;
  struct nsm_msg_control_adj msg;
  s_int32_t len;

  if (! ca || ! ca->gmif || ! ca->gmif->vr || ! ca->gmif->vrf)
    return 0;

  vr = ca->gmif->vr;
  iv = ca->gmif->vrf;

  /* Check requested service */
  if (! nsm_service_check (nse, NSM_SERVICE_CONTROL_ADJ))
    return 0;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_control_adj));
  nsm_server_control_adj_msg_set (ca, cindex, &msg);

  /* Set encode pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  if (IS_NSM_DEBUG_SEND)
    {
      zlog_info (nse->ns->zg, "NSM SEND Control Channel");
      nsm_control_adj_dump (nse->ns->zg, &msg);
    }

  /* Encode NSM service. */
  len = nsm_encode_control_adj (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  /* Send it to client */
  nsm_server_send_message (nse, vr->id, iv->id,
                           NSM_MSG_CONTROL_ADJ, 0, len);
  return 0;
}

s_int32_t
nsm_server_send_control_channel_bundle_msg (u_int32_t vr_id, vrf_id_t vrf_id,
                                            struct nsm_msg_control_channel *msg,
                                            struct nsm_server_entry *nse)
{
  u_char buf[NSM_MESSAGE_MAX_LEN];
  u_int16_t size;
  u_char *pnt = NULL;
  s_int32_t nbytes;
  struct nsm_server_entry_buf *temp_buf = NULL;
  struct nsm_server_bulk_buf *temp_server_buf = NULL;
  struct nsm_server *ns = ng->server;

  temp_server_buf = &(ns->server_buf);
  temp_buf = &(temp_server_buf->bulk_buf);

  /* Set temporary pnt and size.  */
  pnt = buf;
  size = NSM_MESSAGE_MAX_LEN;

  if (IS_NSM_DEBUG_SEND)
    {
      zlog_info (nse->ns->zg, "NSM SEND Control Channel");
      nsm_control_channel_dump (nse->ns->zg, msg);
    }

  /* Encode control channel bundle message. */
  nbytes = nsm_encode_control_channel (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  /* When bundle buffer is filled in flush it.  */
  if (nbytes >= temp_buf->len)
    {
      nsm_server_send_gmplsif_flush (vr_id, vrf_id, &ns->server_buf, nse,
                                     NSM_MSG_CONTROL_CHANNEL);
    }

  /* Set pnt and size.  */
  if (temp_buf->pnt)
    {
      pnt = temp_buf->pnt;
      size = temp_buf->len;
    }
  else
    {
      pnt = temp_buf->buf + NSM_MSG_HEADER_SIZE;
      size = temp_buf->len - NSM_MSG_HEADER_SIZE;
    }

  /* Copy the information.  */
  pal_mem_cpy (pnt, buf, nbytes);

  /* Update pointer and length.  */
  temp_buf->pnt = pnt + nbytes;
  temp_buf->len = size -= nbytes;

  return 0;
}

s_int32_t
nsm_server_send_control_channel_bundle (void_t *info, void_t *arg)
{
  struct gmpls_if *gmif;
  struct control_channel *cc;
  struct nsm_msg_control_channel msg;
  struct nsm_server_entry *nse;
  cindex_t cindex = 0;

  if ((cc = (struct control_channel *)info) == NULL)
    return -1;
  if ((nse = (struct nsm_server_entry *)arg) == NULL)
    return -1;
  if ((gmif = cc->gmif) == NULL)
    return -1;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_control_channel));
  NSM_SET_CTYPE (cindex, NSM_CONTROL_CHANNEL_CTYPE_ADD);
  nsm_server_control_channel_msg_set (cc, cindex, &msg);
  nsm_server_send_control_channel_bundle_msg (gmif->vr->id,
                                              gmif->vrf->id,
                                              &msg, nse);

  return 0;
}

s_int32_t
nsm_server_send_gmpls_control_channel_sync (struct nsm_master *nm,
                                            struct nsm_server_entry *nse)
{
  struct gmpls_if *gmif;
  struct nsm_server *ns = ng->server;

  gmif = gmpls_if_get (nm->zg, nm->vr->id);
  if (! gmif)
    return 0;

  avl_traverse (&gmif->cctree, nsm_server_send_control_channel_bundle, nse);

  /* Send rest of bundled message */
  nsm_server_send_gmplsif_flush (nm->vr->id, gmif->vrf->id, &ns->server_buf,
                                 nse, NSM_MSG_CONTROL_CHANNEL);

  return 0;
}

int nsm_server_recv_control_channel (struct nsm_msg_header *header,
                                     void *arg, void *message)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse, *nse_tmp;
  struct nsm_server *ns;
  struct nsm_msg_control_channel *msg;
  struct gmpls_if *gmif;
  struct control_channel *cc;
  int update_ca = 0;
  cindex_t cindex = 0;
  int i;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = (struct nsm_msg_control_channel *) message;

  if (IS_NSM_DEBUG_RECV)
    nsm_control_channel_dump (ns->zg, msg);

  gmif = gmpls_if_get (ns->zg, header->vr_id);
  if (! gmif)
    return -1;

  cc = gmpls_control_channel_lookup_by_index (gmif, msg->gifindex);
  if (! cc)
    return -1;

  if (CHECK_FLAG (cc->status, GMPLS_INTERFACE_LMP_UP) !=
      CHECK_FLAG (msg->status, GMPLS_INTERFACE_LMP_UP))
    {
      if (CHECK_FLAG (cc->status, GMPLS_INTERFACE_LMP_UP) &&
          ! CHECK_FLAG (msg->status, GMPLS_INTERFACE_LMP_UP))
        {
          if (GMPLS_IF_UP (cc))
            {
              GMPLS_CONTROL_CHANNEL_SET_STATUS_DOWN(cc);
              if (cc->ca)
                {
                  cc->ca->ccupcntrs--;
                  update_ca = 1;
                }
            }
          UNSET_FLAG (cc->status, GMPLS_INTERFACE_LMP_UP);
        }
      if (! CHECK_FLAG (cc->status, GMPLS_INTERFACE_LMP_UP)&&
          CHECK_FLAG (msg->status, GMPLS_INTERFACE_LMP_UP))
        {
          SET_FLAG (cc->status, GMPLS_INTERFACE_LMP_UP);
          if (GMPLS_IF_UP (cc))
            {
              GMPLS_CONTROL_CHANNEL_SET_STATUS_UP(cc);
              if (cc->ca)
                {
                  cc->ca->ccupcntrs++;
                  update_ca = 1;
                }
            }
        }

      if (CHECK_FLAG (cc->status, GMPLS_INTERFACE_GOING_DOWN) &&
          ! CHECK_FLAG (msg->status, GMPLS_INTERFACE_GOING_DOWN))
        UNSET_FLAG (cc->status, GMPLS_INTERFACE_GOING_DOWN);

      if (! CHECK_FLAG (cc->status, GMPLS_INTERFACE_GOING_DOWN) &&
          CHECK_FLAG (msg->status, GMPLS_INTERFACE_GOING_DOWN))
        SET_FLAG (cc->status, GMPLS_INTERFACE_GOING_DOWN);

      /* Notify all clients which registered for Control Channel service,
       * except sender of received msg */
      NSM_SET_CTYPE (cindex, NSM_CONTROL_CHANNEL_CTYPE_STATUS);
      NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse_tmp)
        if (nsm_service_check (nse_tmp, NSM_SERVICE_CONTROL_CHANNEL) &&
            nse_tmp != nse)
          nsm_server_send_gmpls_control_channel (gmif->vr->id, gmif->vrf->id,
                                                 nse_tmp, cc, cindex);

      /* If this control_channel is in-use in control_adj, update ca and
       * send to RSVP */
      if (cc->ca && (cc->gifindex == cc->ca->ccgifindex || update_ca))
        {
          gmpls_control_adj_used_cc_set (cc->ca);

          /* If control adj used another cc, need notify interested client */
          if (cc->ca->ccgifindex != cc->gifindex || update_ca)
            nsm_gmpls_control_adj_send_check (cc->ca, PAL_TRUE);
        }
    }

  return 0;
}

void
nsm_server_control_adj_msg_set (struct control_adj *ca,
                                cindex_t cindex,
                                struct nsm_msg_control_adj *msg)
{
  struct control_channel *cc;

  msg->cindex = cindex;
  pal_mem_cpy (msg->name, ca->name, GINTERFACE_NAMSIZ);
  msg->gifindex = ca->gifindex;
  msg->flags = ca->flags;
  pal_mem_cpy (&msg->r_nodeid, &ca->r_nodeid, sizeof (struct pal_in4_addr));

#ifdef HAVE_LMP
  msg->lmp_inuse = ca->lmp_inuse;
#endif /* HAVE_LMP */
  /* Number of UP cc in ca */
  msg->ccupcntrs = ca->ccupcntrs;

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_ADJ_CTYPE_USE_CC))
    {
      /* Information of control channel which is in use */
      cc = gmpls_control_adj_used_cc_lookup (ca);
      if (cc)
        {
          pal_strncpy (msg->ucc.name, cc->name, GINTERFACE_NAMSIZ);
          msg->ucc.gifindex = cc->gifindex;
          msg->ucc.ccid = cc->ccid;
          msg->ucc.bifindex = cc->bifindex;
          pal_mem_cpy (&msg->ucc.l_addr, &cc->l_addr,
                       sizeof (struct pal_in4_addr));
          pal_mem_cpy (&msg->ucc.r_addr, &cc->r_addr,
                       sizeof (struct pal_in4_addr));
        }
      else
        NSM_UNSET_CTYPE (msg->cindex, NSM_CONTROL_ADJ_CTYPE_USE_CC);
    }
}

s_int32_t
nsm_server_send_control_adj (u_int32_t vr_id, vrf_id_t vrf_id,
                             struct nsm_server_entry *nse,
                             struct control_adj *ca,
                             cindex_t cindex)
{
  struct apn_vr *vr;
  struct apn_vrf *iv;
  struct nsm_msg_control_adj msg;
  s_int32_t len;

  if (! ca || ! ca->gmif || ! ca->gmif->vr || ! ca->gmif->vrf)
    return 0;

  vr = ca->gmif->vr;
  iv = ca->gmif->vrf;

  /* Check requested service */
  if (! nsm_service_check (nse, NSM_SERVICE_CONTROL_ADJ))
    return 0;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_control_adj));
  nsm_server_control_adj_msg_set (ca, cindex, &msg);

  /* Set encode pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  if (IS_NSM_DEBUG_SEND)
    {
      zlog_info (nse->ns->zg, "NSM SEND Control Adjacency");
      nsm_control_adj_dump (nse->ns->zg, &msg);
    }

  /* Encode NSM service. */
  len = nsm_encode_control_adj (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  /* Send it to client */
  nsm_server_send_message (nse, vr->id, iv->id,
                           NSM_MSG_CONTROL_ADJ, 0, len);
  return 0;
}

s_int32_t
nsm_server_send_control_adj_bundle_msg (u_int32_t vr_id, vrf_id_t vrf_id,
                                        struct nsm_msg_control_adj *msg,
                                        struct nsm_server_entry *nse)
{
  u_char buf[NSM_MESSAGE_MAX_LEN];
  u_int16_t size;
  u_char *pnt = NULL;
  s_int32_t nbytes;
  struct nsm_server_entry_buf *temp_buf = NULL;
  struct nsm_server_bulk_buf *temp_server_buf = NULL;
  struct nsm_server *ns = ng->server;

  temp_server_buf = &(ns->server_buf);
  temp_buf = &(temp_server_buf->bulk_buf);

  /* Set temporary pnt and size.  */
  pnt = buf;
  size = NSM_MESSAGE_MAX_LEN;

  if (IS_NSM_DEBUG_SEND)
    {
      zlog_info (nse->ns->zg, "NSM SEND Control Channel");
      nsm_control_adj_dump (nse->ns->zg, msg);
    }

  /* Encode control channel bundle message. */
  nbytes = nsm_encode_control_adj (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  /* When bundle buffer is filled in flush it.  */
  if (nbytes >= temp_buf->len)
    {
      nsm_server_send_gmplsif_flush (vr_id, vrf_id, &ns->server_buf, nse,
                                     NSM_MSG_CONTROL_ADJ);
    }

  /* Set pnt and size.  */
  if (temp_buf->pnt)
    {
      pnt = temp_buf->pnt;
      size = temp_buf->len;
    }
  else
    {
      pnt = temp_buf->buf + NSM_MSG_HEADER_SIZE;
      size = temp_buf->len - NSM_MSG_HEADER_SIZE;
    }

  /* Copy the information.  */
  pal_mem_cpy (pnt, buf, nbytes);

  /* Update pointer and length.  */
  temp_buf->pnt = pnt + nbytes;
  temp_buf->len = size -= nbytes;

  return 0;
}

s_int32_t
nsm_server_send_control_adj_bundle (void_t *info, void_t *arg)
{
  struct gmpls_if *gmif;
  struct control_adj *ca;
  struct nsm_msg_control_adj msg;
  struct nsm_server_entry *nse;
  cindex_t cindex = 0;

  if ((ca = (struct control_adj *)info) == NULL)
    return -1;

  if ((nse = (struct nsm_server_entry *)arg) == NULL)
    return -1;

  if ((gmif = ca->gmif) == NULL)
    return -1;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_control_adj));
  NSM_SET_CTYPE (cindex, NSM_CONTROL_ADJ_CTYPE_ADD);
  if (ca->ccupcntrs > 0)
    NSM_SET_CTYPE (cindex, NSM_CONTROL_ADJ_CTYPE_USE_CC);

  nsm_server_control_adj_msg_set (ca, cindex, &msg);
  nsm_server_send_control_adj_bundle_msg (gmif->vr->id,
                                          gmif->vrf->id,
                                          &msg, nse);
  return 0;
}

s_int32_t
nsm_server_send_gmpls_control_adj_sync (struct nsm_master *nm,
                                        struct nsm_server_entry *nse)
{
  struct gmpls_if *gmif;
  struct nsm_server *ns = ng->server;

  gmif = gmpls_if_get (nm->zg, nm->vr->id);
  if (! gmif)
    return 0;

  avl_traverse (&gmif->catree, nsm_server_send_control_adj_bundle, nse);

  /* Send rest of bundled message */
  nsm_server_send_gmplsif_flush (nm->vr->id, gmif->vrf->id, &ns->server_buf,
                                 nse, NSM_MSG_CONTROL_ADJ);

  return 0;
}

s_int32_t
nsm_server_recv_control_adj (struct nsm_msg_header *header,
                             void *arg, void *message)
{
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_control_adj *msg;
  struct gmpls_if *gmif;
  struct control_adj *ca;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = (struct nsm_msg_control_adj *) message;

  if (IS_NSM_DEBUG_RECV)
    nsm_control_adj_dump (ns->zg, msg);

  gmif = gmpls_if_get (ns->zg, header->vr_id);
  if (! gmif)
    return -1;

  ca = gmpls_control_adj_lookup_by_index (gmif, msg->gifindex);
  if (! ca)
    return -1;

  return 0;
}

s_int32_t
nsm_server_recv_gen_label_pool_request (struct nsm_msg_header *header,
                                         void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_generic_label_pool *msg;
  struct nsm_msg_generic_label_pool reply;
  u_int16_t label_space;
  u_char protocol;
  s_int32_t ret = 0;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;
  label_space = msg->label_space;
  protocol = nse->service.protocol_id;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Prepare reply.  */
  pal_mem_set (&reply, 0, sizeof (struct nsm_msg_generic_label_pool));
  reply.label_space = msg->label_space;

#ifdef HAVE_PBB_TE
  /* Is this a PBB-TE label request ? */
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LABEL_POOL_CTYPE_PBB_TE_LABEL))
  {
    struct pbb_te_label new_label;
    struct nsm_glabel_pbb_te_service_data pbb_data;
    struct nsm_glabel_pbb_te_service_data *tmp_pbb_data;
    u_int32_t tesid = 0;
 
    tmp_pbb_data = NULL;
    pal_mem_set (&new_label, 0, sizeof (struct pbb_te_label));
    pal_mem_set (&pbb_data, 0, sizeof (struct nsm_glabel_pbb_te_service_data));

#if defined HAVE_I_BEB && defined HAVE_B_BEB
#if defined HAVE_GMPLS && defined HAVE_GELS
    /* Call the PBB-TE API's */
    tmp_pbb_data = msg->u.pb_te_service_data;
    if((tmp_pbb_data) &&
       (tmp_pbb_data->lbl_req_type == label_request_ingress))
      ret = nsm_pbb_te_request_label_with_name
                  (tmp_pbb_data->data.trunk_name, &new_label, &tesid);
    else if((tmp_pbb_data) && 
            (tmp_pbb_data->lbl_req_type == label_request_egress))
      ret = nsm_pbb_te_request_label_with_rmac(
                   tmp_pbb_data->pbb_label.bmac,
                   tmp_pbb_data->pbb_label.bvid,&new_label, &tesid);
    else
      ret = -1;
#endif
#endif

    pal_mem_cpy(&pbb_data.pbb_label, &new_label, sizeof(struct pbb_te_label));
    pbb_data.tesid = tesid;
    pbb_data.result = ret;
    reply.u.pb_te_service_data =
                (struct nsm_glabel_pbb_te_service_data *)&pbb_data;
    NSM_SET_CTYPE (reply.cindex, NSM_LABEL_POOL_CTYPE_PBB_TE_LABEL);
  }
#endif /* HAVE_PBB_TE  */

  /* Send reply to client.  */
  ret = nsm_server_send_generic_label_pool (header->vr_id, nse, header->message_id,
                                      &reply, NSM_MSG_GENERIC_LABEL_POOL_REQUEST);
  return ret;
}

s_int32_t
nsm_server_recv_gen_label_pool_release (struct nsm_msg_header *header,
                                    void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_generic_label_pool *msg;
  struct nsm_msg_generic_label_pool reply;
  u_int16_t label_space;
  u_char protocol;
#ifdef HAVE_PBB_TE
  s_int32_t ret = 0;
#endif /* HAVE_PBB_TE */

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;
  label_space = msg->label_space;
  protocol = nse->service.protocol_id;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Prepare reply.  */
  pal_mem_set (&reply, 0, sizeof (struct nsm_msg_generic_label_pool));
  reply.label_space = msg->label_space;

#ifdef HAVE_PBB_TE
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LABEL_POOL_CTYPE_PBB_TE_LABEL))
  {
    struct pbb_te_label *label;
    struct nsm_glabel_pbb_te_service_data pbb_data;
    struct nsm_glabel_pbb_te_service_data *tmp_pbb_data;

    label = NULL;
    tmp_pbb_data = NULL;
    pal_mem_set (&pbb_data, 0, sizeof (struct nsm_glabel_pbb_te_service_data));

#if defined HAVE_I_BEB && defined HAVE_B_BEB
#if defined HAVE_GMPLS && defined HAVE_GELS
    /* Call the PBB-TE API's */
    tmp_pbb_data = msg->u.pb_te_service_data;
    label = &tmp_pbb_data->pbb_label;
    if((tmp_pbb_data) && 
       (label) && 
       ((tmp_pbb_data->lbl_req_type == label_request_ingress) ||
        (tmp_pbb_data->lbl_req_type == label_request_egress)))
      ret = nsm_pbb_te_release_label((nsm_pbb_tesid_t)tmp_pbb_data->tesid,
                                     label->bmac, label->bvid);
#endif
#endif

    pbb_data.result = ret;
    reply.u.pb_te_service_data =
                (struct nsm_glabel_pbb_te_service_data *)&pbb_data;
    NSM_SET_CTYPE (reply.cindex, NSM_LABEL_POOL_CTYPE_PBB_TE_LABEL);
  }
#endif /* HAVE_PBB_TE */

  /* Send reply to client.  */
  nsm_server_send_generic_label_pool (header->vr_id, nse, header->message_id,
                              &reply, NSM_MSG_GENERIC_LABEL_POOL_RELEASE);

  return 0;
}

s_int32_t
nsm_server_recv_gen_label_pool_validate(struct nsm_msg_header *header,
                                    void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_generic_label_pool *msg;
  struct nsm_msg_generic_label_pool reply;
  u_int16_t label_space;
  u_char protocol;
  s_int32_t ret = 0;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;
  label_space = msg->label_space;
  protocol = nse->service.protocol_id;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Prepare reply.  */
  pal_mem_set (&reply, 0, sizeof (struct nsm_msg_generic_label_pool));
  reply.label_space = msg->label_space;

#ifdef HAVE_PBB_TE
  /* Is this a PBB-TE label request ? */
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LABEL_POOL_CTYPE_PBB_TE_LABEL))
  {
    struct pbb_te_label *label;
#if defined HAVE_GMPLS && defined HAVE_GELS
    struct interface *ifp = NULL;
#endif /* HAVE_GMPLS && defined HAVE_GELS */
    struct nsm_glabel_pbb_te_service_data pbb_data;
    struct nsm_glabel_pbb_te_service_data *tmp_pbb_data;

    pal_mem_set (&pbb_data, 0, sizeof (struct nsm_glabel_pbb_te_service_data));

    /* Call the PBB-TE API's */
    tmp_pbb_data = msg->u.pb_te_service_data;
    label = &tmp_pbb_data->pbb_label;
    /* Currently label validation is done only in the Transit nodes */
    if(tmp_pbb_data->lbl_req_type == label_request_transit)
      {
#if defined HAVE_GMPLS && defined HAVE_GELS
         /* Get the interface structure from the ifindex */
         NSM_MPLS_GET_INDEX_PTR (PAL_TRUE, tmp_pbb_data->data.ifIndex, ifp);
         if(ifp)
           ret = nsm_pbb_te_action_validate(ifp, label->bmac, label->bvid);
#endif /* HAVE_GMPLS && HAVE_GELS */
       }
     else if(tmp_pbb_data->lbl_req_type == label_request_ingress)
       {
#if defined HAVE_I_BEB && defined HAVE_B_BEB
#if defined HAVE_GMPLS && defined HAVE_GELS
         ret = nsm_pbb_te_beb_validate((nsm_pbb_tesid_t)tmp_pbb_data->tesid,
                                       label->bmac, label->bvid);
#endif /* HAVE_I_BEB && HAVE_B_BEB */
#endif /* HAVE_GMPLS && HAVE_GELS */
       }

     pbb_data.result = ret;
     reply.u.pb_te_service_data =
                     (struct nsm_glabel_pbb_te_service_data *)&pbb_data;
     NSM_SET_CTYPE (reply.cindex, NSM_LABEL_POOL_CTYPE_PBB_TE_LABEL);
  }
#endif /* HAVE_PBB_TE */

  ret = nsm_server_send_generic_label_pool (header->vr_id, nse, header->message_id,
                                      &reply, NSM_MSG_GENERIC_LABEL_POOL_IN_USE);
  return ret;
}

#endif /* HAVE_GMPLS */

s_int32_t
nsm_server_if_clean_list_remove(struct interface *ifp,
                                struct nsm_server_entry *nse)
{
  if((!ifp) || (!nse))
    return -1;

  if(!ifp->clean_pend_resp_list)
     return 0;

  listnode_delete (ifp->clean_pend_resp_list, nse);
  if(ifp->clean_pend_resp_list->count)
     return 0;

  list_free(ifp->clean_pend_resp_list);
  nsm_if_delete_update_done (ifp);
  return 0;
}

s_int32_t
nsm_server_if_clean_list_add(struct interface *ifp,
                                struct nsm_server_entry *nse)
{
  if((!ifp) || (!nse))
    return -1;

  if(!ifp->clean_pend_resp_list)
     ifp->clean_pend_resp_list = list_new();

  if(!ifp->clean_pend_resp_list)
    return -1;

  listnode_add(ifp->clean_pend_resp_list, nse);
  return 0;
}

s_int32_t
nsm_server_recv_if_del_done (struct nsm_msg_header *header,
                             void *arg, void *message)
{
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct interface *ifp;
  struct nsm_msg_link *msg;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = (struct nsm_msg_link *) message;

  ifp = ifg_lookup_by_index (&NSM_ZG->ifg, msg->ifindex);

  if (!ifp)
    {
       zlog_err (NSM_ZG,
                 "Interface not found for ifindex %d.",msg->ifindex);
        return 0;
    }

   return nsm_server_if_clean_list_remove(ifp,nse);
}
#ifdef HAVE_LACPD
s_int32_t
nsm_server_if_lacp_admin_key_update (struct interface *ifp)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  s_int32_t i;
  cindex_t cindex = 0;

  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_LACP_KEY);

  /* Always send name. */
  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_NAME);

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    {
      if (nse->service.protocol_id == APN_PROTO_LACP)
#if 0
        NSM_INTERFACE_CHECK_MEMBERSHIP (ifp, APN_PROTO_LACP))
#endif
      {
        if (if_is_running (ifp))
          nsm_server_send_interface_state (nse, ifp, NSM_IF_UP, cindex);
        else
          nsm_server_send_interface_state (nse, ifp, NSM_IF_DOWN, cindex);
      }
    }

  return 0;
}
#endif /* HAVE_LACPD */

#ifdef HAVE_MPLS_FRR
s_int32_t
nsm_server_recv_rsvp_control_packet (struct nsm_msg_header *header,
                                     void *arg, void *message)
{
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_rsvp_ctrl_pkt *ctrl_pkt;
  s_int32_t ret;
  u_int32_t ftn_index, cpkt_id, flags, total_len, seq_num, pkt_size;
  char *pkt;


  /* Set connection and message.*/
  nse = arg;
  ns = nse->ns;
  ctrl_pkt = message;

  ftn_index = ctrl_pkt->ftn_index;
  cpkt_id = ctrl_pkt->cpkt_id;
  flags = ctrl_pkt->flags;
  total_len = ctrl_pkt->total_len;
  seq_num = ctrl_pkt->seq_num;
  pkt_size  = ctrl_pkt->pkt_size;
  pkt       = ctrl_pkt->pkt;

  ret = apn_mpls_fwd_over_tunnel (ftn_index,
                                  cpkt_id,
                                  flags,
                                  total_len,
                                  seq_num,
                                  pkt_size,
                                  pkt);
  if (ret < 0)
    {
      zlog_err (NSM_ZG,
                  "Failed to send RSVP Control Packet with FTN_Index %d",
                  ftn_index);
      return NSM_FAILURE;
    }

  return NSM_SUCCESS;
}
#endif /* HAVE_MPLS_FRR */

/*Read NSM message header.  */
s_int32_t
nsm_server_read_header (struct message_handler *ms, struct message_entry *me,
                        pal_sock_handle_t sock)
{
  s_int32_t nbytes;
  struct nsm_server_entry *nse;

  /* Get NSM server entry from message entry.  */
  nse = me->info;

  /* Reset parser pointer and size. */
  nse->recv.pnt = nse->recv.buf;
  nse->recv.size = 0;

  /* Read NSM message header.  */
  nbytes = readn (sock, nse->recv.buf, NSM_MSG_HEADER_SIZE);

  /* Let message handler to handle disconnect event.  */
  if (nbytes <= 0)
    return nbytes;

  /* Check header length.  If length is smaller than NSM message
     header size close the connection.  */
  if (nbytes != NSM_MSG_HEADER_SIZE)
    return -1;

  /* Record read size.  */
  nse->recv.size = nbytes;

  return nbytes;
}

/* Call back function to read NSM message body.  */
s_int32_t
nsm_server_read_msg (struct message_handler *ms, struct message_entry *me,
                     pal_sock_handle_t sock)
{
  s_int32_t ret;
  s_int32_t type;
  s_int32_t nbytes;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct nsm_msg_header header;

  /* Get NSM server entry from message entry.  */
  nse = me->info;
  ns = nse->ns;

  /* Reset parser pointer and size. */
  nse->recv.pnt = nse->recv.buf;
  nse->recv.size = 0;

  /* Read NSM message header.  */
  nbytes = readn (sock, nse->recv.buf, NSM_MSG_HEADER_SIZE);

  /* Let message handler to handle disconnect event.  */
  if (nbytes <= 0)
    return -1;

  /* Check header length.  If length is smaller than NSM message
     header size close the connection.  */
  if (nbytes != NSM_MSG_HEADER_SIZE)
    return -1;

  /* Record read size.  */
  nse->recv.size = nbytes;

  /* Parse NSM message header.  */
  ret = nsm_decode_header (&nse->recv.pnt, &nse->recv.size, &header);
  if (ret < 0)
    return -1;

  /* Dump NSM header.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_header_dump (me->zg, &header);

  /* Reset parser pointer and size. */
  nse->recv.pnt = nse->recv.buf;
  nse->recv.size = 0;

  /* Read NSM message body.  */
  nbytes = readn (sock, nse->recv.pnt, header.length - NSM_MSG_HEADER_SIZE);

  /* Let message handler to handle disconnect event.  */
  if (nbytes <= 0)
    return -1;

  /* Record read size.  */
  nse->recv.size = nbytes;
  type = header.type;

  /* Increment counter.  */
  nse->recv_msg_count++;

  /* Put last read type.  */
  nse->last_read_type = type;

  /* Invoke call back function.  */
  if (type < NSM_MSG_MAX && ns->parser[type] && ns->callback[type])
    {
      ret = (*ns->parser[type]) (&nse->recv.pnt, &nse->recv.size, &header, nse,
                                 ns->callback[type]);
      if (ret < 0)
        return ret;
    }

  return nbytes;
}

/* Set protocol version.  */
void
nsm_server_set_version (struct nsm_server *ns, u_int16_t version)
{
  ns->service.version = version;
}

/* Set protocol ID.  */
void
nsm_server_set_protocol (struct nsm_server *ns, u_int32_t protocol_id)
{
  ns->service.protocol_id = protocol_id;
}

struct nsm_server_entry *
nsm_server_lookup_by_proto_id (u_int32_t protocol_id)
{
  struct nsm_server_entry *nse = NULL;
  struct nsm_server_client *nsc;
  s_int32_t i;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    {
      if (nse->service.protocol_id == protocol_id)
        return nse;
    }

  return nse;
}

/* Initialize NSM server.  */
struct nsm_server *
nsm_server_init (struct lib_globals *zg)
{
  s_int32_t ret;
  struct nsm_server *ns;
  struct message_handler *ms;

  /* Create message server.  */
  ms = message_server_create (zg);
  if (! ms)
    return NULL;

#ifndef HAVE_TCP_MESSAGE
  /* Set server type to UNIX domain socket.  */
  message_server_set_style_domain (ms, NSM_SERV_PATH);
#else /* HAVE_TCP_MESSAGE */
  message_server_set_style_tcp (ms, NSM_PORT);
#endif /* !HAVE_TCP_MESSAGE */

  /* Set call back functions.  */
  message_server_set_callback (ms, MESSAGE_EVENT_CONNECT,
                               nsm_server_connect);
  message_server_set_callback (ms, MESSAGE_EVENT_DISCONNECT,
                               nsm_server_disconnect);
  message_server_set_callback (ms, MESSAGE_EVENT_READ_MESSAGE,
                               nsm_server_read_msg);

  /* Start NSM server.  */
  ret = message_server_start (ms);
  if (ret < 0)
    ;

  /* When message server works fine, go forward to create NSM server
     structure.  */
  ns = XCALLOC (MTYPE_NSM_SERVER, sizeof (struct nsm_server));
  ns->zg = zg;
  ns->ms = ms;
  ms->info = ns;
  ns->client = vector_init (1);

  pal_mem_set (&ns->server_buf, 0, sizeof (struct nsm_server_bulk_buf));
  ns->server_buf.bulk_buf.len= NSM_MESSAGE_MAX_LEN;

  /* Set version and protocol.  */
  nsm_server_set_version (ns, NSM_PROTOCOL_VERSION_1);
  nsm_server_set_protocol (ns, APN_PROTO_NSM);

  /* Set services.  */
  nsm_server_set_service (ns, NSM_SERVICE_INTERFACE);
  nsm_server_set_service (ns, NSM_SERVICE_ROUTE);
  nsm_server_set_service (ns, NSM_SERVICE_ROUTER_ID);
  nsm_server_set_service (ns, NSM_SERVICE_VRF);
  nsm_server_set_service (ns, NSM_SERVICE_ROUTE_LOOKUP);
  nsm_server_set_service (ns, NSM_SERVICE_LABEL);
#ifdef HAVE_TE
  nsm_server_set_service (ns, NSM_SERVICE_TE);
#endif /* HAVE_TE */
  nsm_server_set_service (ns, NSM_SERVICE_QOS);
  nsm_server_set_service (ns, NSM_SERVICE_QOS_PREEMPT);
  nsm_server_set_service (ns, NSM_SERVICE_USER);
  nsm_server_set_service (ns, NSM_SERVICE_MPLS_VC);
  nsm_server_set_service (ns, NSM_SERVICE_MPLS);
  nsm_server_set_service (ns, NSM_SERVICE_IGP_SHORTCUT);
#ifdef HAVE_GMPLS
  nsm_server_set_service (ns, NSM_SERVICE_GMPLS);
  nsm_server_set_service (ns, NSM_SERVICE_CONTROL_ADJ);
  nsm_server_set_service (ns, NSM_SERVICE_CONTROL_CHANNEL);
  nsm_server_set_service (ns, NSM_SERVICE_TE_LINK);
  nsm_server_set_service (ns, NSM_SERVICE_DATA_LINK);
  nsm_server_set_service (ns, NSM_SERVICE_DATA_LINK_SUB);
#endif /* HAVE_GMPLS */
#ifdef HAVE_DIFFSERV
  nsm_server_set_service (ns, NSM_SERVICE_DIFFSERV);
#endif /* HAVE_DIFFSERV */
#ifdef HAVE_VPLS
  nsm_server_set_service (ns, NSM_SERVICE_VPLS);
#endif /* HAVE_VPLS */
#ifdef HAVE_DSTE
  nsm_server_set_service (ns, NSM_SERVICE_DSTE);
#endif /* HAVE_DSTE */
 #ifdef HAVE_L2
  nsm_server_set_service (ns, NSM_SERVICE_BRIDGE);
#ifdef HAVE_VLAN
  nsm_server_set_service (ns, NSM_SERVICE_VLAN);
#endif /* HAVE_VLAN */

#endif /* HAVE_L2 */

  /* Set callback functions to NSM.  */
  nsm_server_set_callback (ns, NSM_MSG_SERVICE_REQUEST,
                           nsm_parse_service,
                           nsm_server_recv_service);
#ifdef HAVE_L3
  nsm_server_set_callback (ns, NSM_MSG_REDISTRIBUTE_SET,
                           nsm_parse_redistribute,
                           nsm_server_recv_redistribute_set);
  nsm_server_set_callback (ns, NSM_MSG_REDISTRIBUTE_UNSET,
                           nsm_parse_redistribute,
                           nsm_server_recv_redistribute_unset);
  nsm_server_set_callback (ns, NSM_MSG_ISIS_WAIT_BGP_SET,
                           nsm_parse_bgp_conv,
                           nsm_server_recv_isis_wait_bgp_set);
  nsm_server_set_callback (ns, NSM_MSG_BGP_CONV_DONE,
  			   nsm_parse_bgp_conv,
  			   nsm_server_recv_bgp_conv_done);
  nsm_server_set_callback (ns, NSM_MSG_LDP_SESSION_STATE,
                           nsm_parse_ldp_session_state,
                           nsm_server_recv_ldp_session_state);
  nsm_server_set_callback (ns, NSM_MSG_LDP_SESSION_QUERY,
                           nsm_parse_ldp_session_state,
                           nsm_server_recv_ldp_session_query);
#endif /* HAVE_L3 */

#ifdef HAVE_CRX
  nsm_server_set_callback (ns, NSM_MSG_ADDR_ADD, nsm_parse_address,
                           nsm_server_recv_interface_addr_add);
  nsm_server_set_callback (ns, NSM_MSG_ADDR_DELETE, nsm_parse_address,
                           nsm_server_recv_interface_addr_del);
#endif /* HAVE_CRX */

#ifdef HAVE_L3
  nsm_rib_set_server_callback (ns);
#endif

#ifdef HAVE_CRX
  nsm_server_set_callback (ns, NSM_MSG_ROUTE_CLEAN,
                           nsm_parse_route_clean,
                           nsm_server_recv_route_clean);
  nsm_server_set_callback (ns, NSM_MSG_PROTOCOL_RESTART,
                           nsm_parse_protocol_restart,
                           nsm_server_recv_protocol_restart);
#endif /* HAVE_CRX */

#ifdef HAVE_L3
  nsm_server_set_callback (ns, NSM_MSG_ROUTE_PRESERVE,
                           nsm_parse_route_manage,
                           nsm_server_recv_route_preserve);
  nsm_server_set_callback (ns, NSM_MSG_ROUTE_STALE_REMOVE,
                           nsm_parse_route_manage,
                           nsm_server_recv_route_stale_remove);
#ifdef HAVE_RESTART
  nsm_server_set_callback (ns, NSM_MSG_ROUTE_STALE_MARK,
                           nsm_parse_route_manage,
                           nsm_server_recv_route_stale_mark);
#endif /* HAVE_RESTART */

#endif /* HAVE_L3 */

#ifdef HAVE_MPLS
  IF_NSM_CAP_HAVE_MPLS
    {
      nsm_server_set_callback (ns, NSM_MSG_LABEL_POOL_REQUEST,
                               nsm_parse_label_pool,
                               nsm_server_recv_label_pool_request);
      nsm_server_set_callback (ns, NSM_MSG_LABEL_POOL_RELEASE,
                               nsm_parse_label_pool,
                               nsm_server_recv_label_pool_release);
#ifdef HAVE_PACKET
      nsm_server_set_callback (ns, NSM_MSG_ILM_LOOKUP,
                               nsm_parse_ilm_lookup,
                               nsm_server_recv_ilm_lookup);
#endif /* HAVE_PACKET */
    }
#endif /* HAVE_MPLS */
#ifdef HAVE_TE
  nsm_server_set_callback (ns, NSM_MSG_QOS_CLIENT_INIT,
                           nsm_parse_qos_client_init,
                           nsm_read_qos_client_init);
  nsm_server_set_callback (ns, NSM_MSG_QOS_CLIENT_PROBE,
                           nsm_parse_qos,
                           nsm_read_qos_client_probe);
  nsm_server_set_callback (ns, NSM_MSG_QOS_CLIENT_RESERVE,
                           nsm_parse_qos,
                           nsm_read_qos_client_reserve);
  nsm_server_set_callback (ns, NSM_MSG_QOS_CLIENT_MODIFY,
                           nsm_parse_qos,
                           nsm_read_qos_client_modify);
  nsm_server_set_callback (ns, NSM_MSG_QOS_CLIENT_RELEASE,
                           nsm_parse_qos_release,
                           nsm_read_qos_client_release);
  nsm_server_set_callback (ns, NSM_MSG_QOS_CLIENT_CLEAN,
                           nsm_parse_qos_clean,
                           nsm_read_qos_client_clean);
#ifdef HAVE_GMPLS
  nsm_server_set_callback (ns, NSM_MSG_GMPLS_QOS_CLIENT_INIT,
                           nsm_parse_gmpls_qos_client_init,
                           nsm_read_gmpls_qos_client_init);
  nsm_server_set_callback (ns, NSM_MSG_GMPLS_QOS_CLIENT_PROBE,
                           nsm_parse_gmpls_qos,
                           nsm_read_gmpls_qos_client_probe);
  nsm_server_set_callback (ns, NSM_MSG_GMPLS_QOS_CLIENT_RESERVE,
                           nsm_parse_gmpls_qos,
                           nsm_read_gmpls_qos_client_reserve);
  nsm_server_set_callback (ns, NSM_MSG_GMPLS_QOS_CLIENT_MODIFY,
                           nsm_parse_gmpls_qos,
                           nsm_read_gmpls_qos_client_modify);
  nsm_server_set_callback (ns, NSM_MSG_GMPLS_QOS_CLIENT_RELEASE,
                           nsm_parse_gmpls_qos_release,
                           nsm_read_gmpls_qos_client_release);
  nsm_server_set_callback (ns, NSM_MSG_GMPLS_QOS_CLIENT_CLEAN,
                           nsm_parse_gmpls_qos_clean,
                           nsm_read_gmpls_qos_client_clean);
#endif /* HAVE_GMPLS */
#endif /* HAVE_TE */
#ifdef HAVE_GMPLS
  nsm_server_set_callback (ns, NSM_MSG_ILM_GEN_LOOKUP,
                               nsm_parse_ilm_gen_lookup,
                               nsm_server_recv_ilm_gen_lookup);

  nsm_server_set_callback (ns, NSM_MSG_DATA_LINK,
                           nsm_parse_data_link,
                           nsm_read_data_link);

  nsm_server_set_callback (ns, NSM_MSG_DATA_LINK_SUB,
                           nsm_parse_data_link_sub,
                           nsm_read_data_link_sub);

  nsm_server_set_callback (ns, NSM_MSG_TE_LINK,
                           nsm_parse_te_link,
                           nsm_read_te_link);

  nsm_server_set_callback (ns, NSM_MSG_GENERIC_LABEL_POOL_REQUEST,
                               nsm_parse_generic_label_pool,
                               nsm_server_recv_gen_label_pool_request);
  nsm_server_set_callback (ns, NSM_MSG_GENERIC_LABEL_POOL_RELEASE,
                               nsm_parse_generic_label_pool,
                               nsm_server_recv_gen_label_pool_release);
  nsm_server_set_callback (ns, NSM_MSG_GENERIC_LABEL_POOL_IN_USE,
                               nsm_parse_generic_label_pool,
                               nsm_server_recv_gen_label_pool_validate);

  nsm_server_set_callback (ns, NSM_MSG_CONTROL_CHANNEL,
                               nsm_parse_control_channel,
                               nsm_server_recv_control_channel);

  nsm_server_set_callback (ns, NSM_MSG_CONTROL_ADJ,
                               nsm_parse_control_adj,
                               nsm_server_recv_control_adj);

#endif /* HAVE_GMPLS */
#ifdef HAVE_MPLS
  IF_NSM_CAP_HAVE_MPLS
    {
#ifdef HAVE_PACKET
      nsm_server_set_callback (ns, NSM_MSG_MPLS_FTN_IPV4,
                               nsm_parse_ftn_ipv4,
                               nsm_server_recv_ftn_ipv4);
#ifdef HAVE_IPV6
      nsm_server_set_callback (ns, NSM_MSG_MPLS_FTN_IPV6,
                               nsm_parse_ftn_ipv6,
                               nsm_server_recv_ftn_ipv6);
#endif
      nsm_server_set_callback (ns, NSM_MSG_MPLS_ILM_IPV4,
                               nsm_parse_ilm_ipv4,
                               nsm_server_recv_ilm_ipv4);
#ifdef HAVE_IPV6
      nsm_server_set_callback (ns, NSM_MSG_MPLS_ILM_IPV6,
                               nsm_parse_ilm_ipv6,
                               nsm_server_recv_ilm_ipv6);
#endif /* HAVE_IPV6 */
#endif /* HAVE_PACKET */
      nsm_server_set_callback (ns, NSM_MSG_MPLS_FTN_GEN_IPV4,
                               nsm_parse_ftn_gen_data,
                               nsm_server_recv_ftn_gen_ipv4);

      nsm_server_set_callback (ns, NSM_MSG_MPLS_ILM_GEN_IPV4,
                               nsm_parse_ilm_gen_data,
                               nsm_server_recv_ilm_gen_ipv4);

      nsm_server_set_callback (ns, NSM_MSG_MPLS_IGP_SHORTCUT_ROUTE,
                               nsm_parse_igp_shortcut_route,
                               nsm_server_recv_igp_shortcut_route);
#if defined HAVE_MPLS_OAM || defined HAVE_NSM_MPLS_OAM
      nsm_server_set_callback (ns, NSM_MSG_MPLS_OAM_L3VPN,
                               nsm_parse_mpls_vpn_rd,
                               nsm_server_recv_mpls_vpn_rd);
#endif /* HAVE_MPLS_OAM || HAVE_NSM_MPLS_OAM */
    }
#endif /* HAVE_MPLS */

#ifdef HAVE_GMPLS
  nsm_server_set_callback (ns, NSM_MSG_GMPLS_BIDIR_FTN,
                           nsm_parse_gmpls_ftn_bidir_data,
                           nsm_server_recv_gmpls_bidir_ftn);

  nsm_server_set_callback (ns, NSM_MSG_GMPLS_BIDIR_ILM,
                           nsm_parse_gmpls_ilm_bidir_data,
                           nsm_server_recv_gmpls_bidir_ilm);
#endif /*HAVE_GMPLS*/

#ifdef HAVE_MPLS_VC
  nsm_server_set_callback (ns, NSM_MSG_MPLS_VC_FIB_ADD,
                           nsm_parse_vc_fib_add,
                           nsm_server_recv_vc_fib_add);
  nsm_server_set_callback (ns, NSM_MSG_MPLS_VC_FIB_DELETE,
                           nsm_parse_vc_fib_delete,
                           nsm_server_recv_vc_fib_del);
  nsm_server_set_callback (ns, NSM_MSG_MPLS_VC_TUNNEL_INFO,
                           nsm_parse_vc_tunnel_info,
                           nsm_server_recv_tunnel_info);
  nsm_server_set_callback (ns, NSM_MSG_MPLS_PW_STATUS,
                           nsm_parse_pw_status,
                           nsm_server_recv_pw_status);
#endif /* HAVE_MPLS_VC */
#ifdef HAVE_VPLS
  nsm_server_set_callback (ns, NSM_MSG_VPLS_MAC_WITHDRAW,
                           nsm_parse_vpls_mac_withdraw,
                           nsm_server_recv_vpls_mac_withdraw);
#endif /* HAVE_VPLS */

#ifndef HAVE_IMI
  host_user_add_callback (zg, USER_CALLBACK_UPDATE,
                          nsm_server_user_update_callback);
  host_user_add_callback (zg, USER_CALLBACK_DELETE,
                          nsm_server_user_delete_callback);
#endif /* HAVE_IMI */

#ifdef HAVE_MCAST_IPV4
  nsm_server_set_callback (ns, NSM_MSG_IPV4_VIF_ADD,
                           nsm_parse_ipv4_vif_add,
                           nsm_server_recv_ipv4_vif_add);
  nsm_server_set_callback (ns, NSM_MSG_IPV4_VIF_DEL,
                           nsm_parse_ipv4_vif_del,
                           nsm_server_recv_ipv4_vif_del);
  nsm_server_set_callback (ns, NSM_MSG_IPV4_MRT_ADD,
                           nsm_parse_ipv4_mrt_add,
                           nsm_server_recv_ipv4_mrt_add);
  nsm_server_set_callback (ns, NSM_MSG_IPV4_MRT_DEL,
                           nsm_parse_ipv4_mrt_del,
                           nsm_server_recv_ipv4_mrt_del);
  nsm_server_set_callback (ns, NSM_MSG_IPV4_MRT_STAT_FLAGS_UPDATE,
                           nsm_parse_ipv4_mrt_stat_flags_update,
                           nsm_server_recv_ipv4_mrt_stat_flags_update);
  nsm_server_set_callback (ns, NSM_MSG_IPV4_MRT_WHOLEPKT_REPLY,
                           nsm_parse_ipv4_mrt_wholepkt_reply,
                           nsm_server_recv_ipv4_mrt_wholepkt_reply);
  nsm_server_set_callback (ns, NSM_MSG_MTRACE_QUERY,
                           nsm_parse_mtrace_query_msg,
                           nsm_server_recv_mtrace_query_msg);
  nsm_server_set_callback (ns, NSM_MSG_MTRACE_REQUEST,
                           nsm_parse_mtrace_request_msg,
                           nsm_server_recv_mtrace_request_msg);
  nsm_server_set_callback (ns, NSM_MSG_IPV4_MRT_ST_REFRESH_FLAG_UPDATE,
                           nsm_parse_ipv4_mrt_state_refresh_flag_update,
                           nsm_server_recv_ipv4_mrt_state_refresh_flag_update);
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
  nsm_server_set_callback (ns, NSM_MSG_IPV6_MIF_ADD,
                           nsm_parse_ipv6_mif_add,
                           nsm_server_recv_ipv6_mif_add);
  nsm_server_set_callback (ns, NSM_MSG_IPV6_MIF_DEL,
                           nsm_parse_ipv6_mif_del,
                           nsm_server_recv_ipv6_mif_del);
  nsm_server_set_callback (ns, NSM_MSG_IPV6_MRT_ADD,
                           nsm_parse_ipv6_mrt_add,
                           nsm_server_recv_ipv6_mrt_add);
  nsm_server_set_callback (ns, NSM_MSG_IPV6_MRT_DEL,
                           nsm_parse_ipv6_mrt_del,
                           nsm_server_recv_ipv6_mrt_del);
  nsm_server_set_callback (ns, NSM_MSG_IPV6_MRT_STAT_FLAGS_UPDATE,
                           nsm_parse_ipv6_mrt_stat_flags_update,
                           nsm_server_recv_ipv6_mrt_stat_flags_update);
  nsm_server_set_callback (ns, NSM_MSG_IPV6_MRT_ST_REFRESH_FLAG_UPDATE,
                           nsm_parse_ipv6_mrt_state_refresh_flag_update,
                           nsm_server_recv_ipv6_mrt_state_refresh_flag_update);
  nsm_server_set_callback (ns, NSM_MSG_IPV6_MRT_WHOLEPKT_REPLY,
                           nsm_parse_ipv6_mrt_wholepkt_reply,
                           nsm_server_recv_ipv6_mrt_wholepkt_reply);
#endif /* HAVE_MCAST_IPV6 */
  nsm_server_set_callback (ns, NSM_MSG_IF_DEL_DONE,
                           nsm_parse_link,
                           nsm_server_recv_if_del_done);

#ifdef HAVE_RMOND
  nsm_server_set_callback (ns, NSM_MSG_RMON_REQ_STATS,
                           nsm_parse_rmon_msg,
                           nsm_server_recv_rmon_req_statistics);
#endif /* HAVE_RMON */

#ifdef HAVE_MPLS_FRR
  nsm_server_set_callback (ns, NSM_MSG_RSVP_CONTROL_PACKET,
                           nsm_parse_rsvp_control_packet,
                           nsm_server_recv_rsvp_control_packet);
#endif /* HAVE_MPLS_FRR */
#ifdef HAVE_NSM_MPLS_OAM
  nsm_server_set_callback (ns, NSM_MSG_MPLS_ECHO_REQUEST,
                           nsm_parse_mpls_oam_request,
                           nsm_server_recv_mpls_oam_req);
#elif defined (HAVE_MPLS_OAM)
/* Set parser & callback functions for echo request/reply process
 * messages from OAMD.
 */
  nsm_server_set_callback (ns, NSM_MSG_OAM_LSP_PING_REQ_PROCESS,
                           nsm_parse_mpls_oam_req_process,
                           nsm_server_recv_mpls_oam_req_process);

  nsm_server_set_callback (ns, NSM_MSG_OAM_LSP_PING_REP_PROCESS_LDP,
                           nsm_parse_mpls_oam_rep_process_ldp,
                           nsm_server_recv_mpls_oam_rep_process_ldp);

  nsm_server_set_callback (ns, NSM_MSG_OAM_LSP_PING_REP_PROCESS_GEN,
                           nsm_parse_mpls_oam_rep_process_gen,
                           nsm_server_recv_mpls_oam_rep_process_gen);

  nsm_server_set_callback (ns, NSM_MSG_OAM_LSP_PING_REP_PROCESS_RSVP,
                           nsm_parse_mpls_oam_rep_process_rsvp,
                           nsm_server_recv_mpls_oam_rep_process_rsvp);

#ifdef HAVE_MPLS_VC
  nsm_server_set_callback (ns, NSM_MSG_OAM_LSP_PING_REP_PROCESS_L2VC,
                           nsm_parse_mpls_oam_rep_process_l2vc,
                           nsm_server_recv_mpls_oam_rep_process_l2vc);
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VRF
  nsm_server_set_callback (ns, NSM_MSG_OAM_LSP_PING_REP_PROCESS_L3VPN,
                           nsm_parse_mpls_oam_rep_process_l3vpn,
                           nsm_server_recv_mpls_oam_rep_process_l3vpn);
#endif /* HAVE_VRF */
#endif /* HAVE_MPLS_OAM */

#ifdef HAVE_MPLS_OAM_ITUT
  nsm_server_set_callback (ns, NSM_MSG_MPLS_OAM_ITUT_PROCESS_REQ,
                           nsm_parse_mpls_oam_process_request,
                           nsm_server_recv_mpls_oam_process_req);
#endif /* HAVE_MPLS_OAM_ITUT */

#ifndef HAVE_CUSTOM1
#ifdef HAVE_L2
  nsm_bridge_set_server_callback (ns);
#endif /* HAVE_L2 */

#ifdef HAVE_VLAN
  nsm_vlan_set_server_callback (ns);
#endif /* HAVE_VLAN */

#ifdef HAVE_AUTHD
  nsm_auth_set_server_callback (ns);

#endif /* HAVE_AUTHD */

#ifdef HAVE_LACPD
  nsm_lacp_set_server_callback (ns);
#endif /* HAVE_LACPD */

#ifdef HAVE_ONMD
  nsm_oam_set_server_callback (ns);
#endif /* HAVE_ONMD */

#ifdef HAVE_ELMID
  nsm_elmi_set_server_callback (ns);
#endif /* HAVE_ELMID  */

#if defined (HAVE_RSTPD) || defined (HAVE_STPD) || defined (HAVE_MSTPD) || defined (HAVE_RPVST_PLUS)
  nsm_stp_set_server_callback (ns);
#endif /* (HAVE_RSTPD) || (HAVE_STPD) || ((HAVE_MSTPD) || HAVE_RPVST_PLUS */
#endif /* HAVE_CUSTOM1 */

  ng->server = ns;

  return ns;
}

s_int32_t
nsm_server_finish (struct nsm_server *ns)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse, *next;
  s_int32_t i;

  /* Clean up the NSM server clients.  */
  for (i = 0; i < vector_max (ns->client); i++)
    if ((nsc = vector_slot (ns->client, i)) != NULL)
      {
        for (nse = nsc->head; nse; nse = next)
          {
            next = nse->next;
            message_entry_free (nse->me);
            nsm_server_entry_free (nse);
          }
        XFREE (MTYPE_NSM_SERVER_CLIENT, nsc);
      }
  vector_free (ns->client);

  message_server_delete (ns->ms);

  XFREE (MTYPE_NSM_SERVER, ns);

  return 0;
}

#ifdef HAVE_NSM_MPLS_OAM
s_int32_t
nsm_server_recv_mpls_oam_req (struct nsm_msg_header *header,
                              void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct nsm_msg_mpls_ping_req *mpls_oam;

  /* Set connection and message */
  nse = arg;
  ns = nse->ns;
  mpls_oam = message;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Process message */
  nsm_mpls_oam_process_echo_request (nm, nse, mpls_oam);
  return 0;
}
#endif /* HAVE_NSM_MPLS_OAM */

#ifdef HAVE_MPLS_OAM_ITUT
s_int32_t
nsm_server_recv_mpls_oam_process_req (struct nsm_msg_header *header,
                              void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct nsm_msg_mpls_oam_itut_req *mpls_oam;

  /* Set connection and message */
  nse = arg;
  ns = nse->ns;
  mpls_oam = message;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Process message */
  nsm_mpls_oam_itut_process_request (nm, nse, mpls_oam);
  return 0;
}
#endif /* HAVE_MPLS_OAM_ITUT*/

#ifdef HAVE_MPLS_OAM
/* Call back function to handle echo request process message from OAMD. */
int
nsm_server_recv_mpls_oam_req_process (struct nsm_msg_header *header,
                                      void *arg, void *message)
{
  struct nsm_master *nm = NULL;
  struct nsm_server *ns = NULL;
  struct nsm_server_entry *nse = NULL;
  struct nsm_msg_oam_lsp_ping_req_process *oam_req = NULL;
  int ret;

  /* Set connection and message */
  nse = arg;
  ns = nse->ns;
  oam_req = message;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Process message */
  ret = nsm_mpls_oam_process_lsp_ping_req (nm, nse, oam_req);
  if (ret < 0)
    {
      nsm_mpls_oam_send_lsp_ping_req_resp_err (nse, oam_req, ret);
    }

  return 0;
}

/* Call back function to handle echo reply process message
 * for LDP type LSP from OAMD.
 */
int
nsm_server_recv_mpls_oam_rep_process_ldp (struct nsm_msg_header *header,
                                          void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct nsm_msg_oam_lsp_ping_rep_process_ldp *oam_rep;

  /* Set connection and message */
  nse = arg;
  ns = nse->ns;
  oam_rep = message;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Process message */
  nsm_mpls_oam_process_lsp_ping_rep_ldp (nm, nse, oam_rep);

  return 0;
}

/* Call back function to handle echo reply process message
 * for RSVP type LSP from OAMD.
 */
int
nsm_server_recv_mpls_oam_rep_process_rsvp (struct nsm_msg_header *header,
                                           void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct nsm_msg_oam_lsp_ping_rep_process_rsvp *oam_rep;

  /* Set connection and message */
  nse = arg;
  ns = nse->ns;
  oam_rep = message;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Process message */
  nsm_mpls_oam_process_lsp_ping_rep_rsvp (nm, nse, oam_rep);

  return 0;
}

/* Call back function to handle echo reply process message
 * for Static LSP from OAMD.
 */
int
nsm_server_recv_mpls_oam_rep_process_gen (struct nsm_msg_header *header,
                                          void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct nsm_msg_oam_lsp_ping_rep_process_gen *oam_rep;

  /* Set connection and message */
  nse = arg;
  ns = nse->ns;
  oam_rep = message;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Process message */
  nsm_mpls_oam_process_lsp_ping_rep_gen (nm, nse, oam_rep);

  return 0;
}

#ifdef HAVE_MPLS_VC
/* Call back function to handle echo reply process message
 * for L2VC type LSP from OAMD.
 */
int
nsm_server_recv_mpls_oam_rep_process_l2vc (struct nsm_msg_header *header,
                                           void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct nsm_msg_oam_lsp_ping_rep_process_l2vc *oam_rep;

  /* Set connection and message */
  nse = arg;
  ns = nse->ns;
  oam_rep = message;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Process message */
  nsm_mpls_oam_process_lsp_ping_rep_l2vc (nm, nse, oam_rep);

  return 0;
}
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VRF
/* Call back function to handle echo reply process message
 * for L3VPN type LSP from OAMD.
 */
int
nsm_server_recv_mpls_oam_rep_process_l3vpn (struct nsm_msg_header *header,
                                            void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct nsm_msg_oam_lsp_ping_rep_process_l3vpn *oam_rep;

  /* Set connection and message */
  nse = arg;
  ns = nse->ns;
  oam_rep = message;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Process message */
  nsm_mpls_oam_process_lsp_ping_rep_l3vpn (nm, nse, oam_rep);

  return 0;
}
#endif /* HAVE_VRF */
#endif /* HAVE_MPLS_OAM */

#ifdef HAVE_HA
/**********************************************************************
 *           Support for the COMMSG communication interface
 *----------------------------------------------------------------
 * _commsg_parse - Parse NSM COMMSG message.
 *---------------------------------------------------------------
 */
static int
_commsg_parse (u_char    **pnt,
               u_int16_t  *len,
               struct nsm_msg_header *header,
               void       *arg,
               NSM_CALLBACK callback)
{
  struct nsm_server_entry *nse = arg;
  struct nsm_server *ns = nse->ns;

  /*  Just call the COMMSG receive method - it will take care
   *  of decoding the COMMSG header and dispatch the message to the
   *  application.
   */
  ns->ns_commsg_recv_cb(ns->ns_commsg_user_ref,
                        nse->service.protocol_id,
                        *pnt,
                        *len);
  return 0;
}

/* nsm_server_read_msg needs this callback to dispatch the parse message */
s_int32_t
_commsg_dummy_callback (struct nsm_msg_header *header, void *arg, void *message)
{
  return 0;
}

/* Find the client. */
struct nsm_server_entry *
_get_server_entry(struct nsm_server *ns, module_id_t mod_id)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry  *nse;
  s_int32_t vix;

  for (vix = 0; vix < vector_max (ns->client); vix++) {
    if ((nsc = vector_slot (ns->client, vix)) != NULL)
    {
      nse = nsc->head;
      if (nse->service.protocol_id == mod_id) {
        /* Only one server entry might exist for any NSM server client!
         * If this is not true - better fail and get it fixed.
         */
        if (nse == NULL || nse->next != NULL) {
          return (NULL);
        }
        return (nse);
      }
    }
  }
  return (NULL);
}


/*----------------------------------------------------------------
 * nsm_server_commsg_init - Register the COMMSG with the nsm_server
 *                          dispatcher.
 *  This is called by the NSM application initialization. It hooks up
 *  the NSM with the COMMSG utility, so the COMMSG can receive its
 *  messages.
 *
 *  ns        - nsm_server structure pointer
 *  recv_cb   - The COMMSG utility receive callback.
 *
 *  NOTE: This function has a prototype specific to the NSM.
 *---------------------------------------------------------------
 */
s_int32_t
nsm_server_commsg_init(struct nsm_server *ns,
                       nsm_msg_commsg_recv_cb_t recv_cb,
                       void *user_ref)
{
  nsm_server_set_callback (ns, NSM_MSG_COMMSG,
                           _commsg_parse,
                           _commsg_dummy_callback);

  ns->ns_commsg_recv_cb = recv_cb;
  ns->ns_commsg_user_ref= user_ref;
  return 0;
}

/*------------------------------------------------------------------
 * nsm_server_commsg_getbuf - Obtain the network buffer.
 *
 *  nsm_srv   - nsm_server pointer
 *  dst_mod_id- must NOT be APN_PROTO_NSM
 *  size      - Requested buffer space in bytes.
 *
 *  NOTE: This function has a generic prototype used on the client and
 *        the server.
 *------------------------------------------------------------------
 */
u_char *
nsm_server_commsg_getbuf(void       *nsm_srv,
                         module_id_t dst_mod_id,
                         u_int16_t   size)
{
  struct nsm_server *ns = (struct nsm_server *)nsm_srv;
  struct nsm_server_entry *nse;

  nse = _get_server_entry(ns, dst_mod_id);
  if (nse == NULL) {
    return (NULL);
  }
  if (size > nse->send.len - NSM_MSG_HEADER_SIZE) {
    return (NULL);
  }
  return (nse->send.buf + NSM_MSG_HEADER_SIZE);
}

/*------------------------------------------------------------------
 * nsm_server_commsg_send - Send the COMMSG
 *
 *  nsm_clt   - nsm_client pointer
 *  dst_mod_id- must NOT be APN_PROTO_NSM
 *  msg_buf   - Ptr to the message buffer
 *  msg_len   - Message length
 *
 *  NOTE: This function has a generic prototype used on the client and
 *        the server.
 *------------------------------------------------------------------
 */
s_int32_t
nsm_server_commsg_send(void       *nsm_srv,
                       module_id_t dst_mod_id,
                       u_char     *msg_buf,
                       u_int16_t   msg_len)
{
  struct nsm_server *ns = (struct nsm_server *)nsm_srv;
  struct nsm_server_entry *nse;

  nse = _get_server_entry(ns, dst_mod_id);
  if (nse == NULL) {
    return (-1);
  }
  /* Validate the buffer pointer. */
  if (msg_buf != (nse->send.buf + NSM_MSG_HEADER_SIZE)) {
    return -1;
  }
  return nsm_server_send_message (nse, 0, 0, NSM_MSG_COMMSG, 0, msg_len);
}
#endif /* HAVE_HA */

void
nsm_imi_config_completed (struct lib_globals *zg)
{
  struct nsm_master *nm = NULL;
  struct nsm_server_entry *nse = NULL;
  struct nsm_server_client *nsc = NULL;
  u_int32_t i;

  if (zg == NULL)
    return;

  if (zg->vr_in_cxt == NULL)
    return;

  nm = zg->vr_in_cxt->proto;
  if (nm == NULL)
    return;

  /* Unsuccessful port mirror list restore. */
#ifdef HAVE_L2
  port_mirror_list_config_restore_after_sync (zg);
#endif /* HAVE_L2 */

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    {
      if (nse->service.protocol_id == APN_PROTO_LACP)
        {
#ifdef HAVE_LACPD
          nsm_server_send_lacp_config_sync (nm, nse);
#endif /* HAVE_LACPD */

          nsm_server_send_vr_sync_done (nse);
        }
      else if (nse->service.protocol_id == APN_PROTO_ONM)
        {
#ifdef HAVE_L2
          nsm_bridge_sync (nm, nse);
#ifdef HAVE_VLAN
          nsm_bridge_vlan_sync (nm, nse);
#endif /* HAVE_VLAN */
#ifdef HAVE_G8031
          nsm_g8031_sync (nm, nse);
#endif /* HAVE_G8031 */

#ifdef HAVE_G8032
          nsm_g8032_sync (nm, nse);
#endif /* HAVE_G8032 */
#endif /* HAVE_L2 */
#if defined HAVE_I_BEB || defined HAVE_B_BEB
          nsm_pbb_sync (nm, nse);
#endif /* HAVE_I_BEB || HAVE_B_BEB */
          nsm_server_send_vr_sync_done (nse);
        }/* if (nse->service.protocol_id == APN_PROTO_ONM) */
#ifdef HAVE_IMI
      else if (nse->service.protocol_id == APN_PROTO_MSTP)
        {
#ifdef HAVE_L2
          nsm_bridge_sync (nm, nse);
#ifdef HAVE_VLAN
          nsm_bridge_vlan_sync (nm, nse);
#endif /* HAVE_VLAN */
#ifdef HAVE_G8031
          nsm_g8031_sync (nm, nse);
#endif /* HAVE_G8031 */

#ifdef HAVE_G8032
  nsm_g8032_sync (nm, nse);
#endif /* HAVE_G8032 */
#endif /* HAVE_L2 */
#if defined HAVE_I_BEB || defined HAVE_B_BEB
          nsm_pbb_sync (nm, nse);
#endif /* HAVE_I_BEB || HAVE_B_BEB */
          nsm_server_send_vr_sync_done (nse);
        } /* APN_PROTO_MSTP */
#endif /* HAVE_IMI */
#ifdef HAVE_ELMID
      else if (nse->service.protocol_id == APN_PROTO_ELMI)
        {
#ifdef HAVE_L2
          nsm_bridge_sync (nm, nse);
#ifdef HAVE_VLAN
          nsm_bridge_vlan_sync (nm, nse);
#endif /* HAVE_VLAN */
          nsm_elmi_sync (nm, nse);

          nsm_server_send_vr_sync_done (nse);

#endif /* HAVE_L2 */
        }/* if (nse->service.protocol_id == APN_PROTO_ELMI) */
#endif /* HAVE_ELMID */

    }/* NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse) */
}

#if defined HAVE_I_BEB || defined HAVE_B_BEB
int nsm_pbb_sync (struct nsm_master *nm, struct nsm_server_entry *nse)
{
  struct nsm_bridge_master *master;
  struct nsm_bridge *bridge = NULL;
  struct avl_node *node;

#ifdef HAVE_I_BEB
  struct nsm_msg_pbb_isid_to_pip isid_to_pip;
  struct cnp_srv_tbl *cnp_node = NULL;
  struct svlan_bundle *svlan_bdl;
  struct listnode *svlan_node;
  struct nsm_pbb_icomponent *icomp = NULL;
#endif /* HAVE_I_BEB */

#ifdef HAVE_B_BEB
  struct nsm_pbb_bcomponent *bcomp = NULL;
  struct cbp_srvinst_tbl *cbp_node = NULL;
  struct nsm_msg_pbb_isid_to_bvid isid_to_bvid;
#ifdef HAVE_I_BEB
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_msg_vlan_port_type port_type_msg;
  struct avl_node *br_node = NULL;
  u_int16_t ifindex_pip = 0;
  u_int16_t ifindex_cbp = 0;
  struct interface *ifp_pip = NULL;
  struct interface *ifp_cbp = NULL;
  u_int8_t logical_ifp = 0;
  cindex_t cindex = 0;
#endif /* I && B */
#endif /* HAVE_B_BEB */

#ifdef HAVE_PBB_TE
  struct pbb_te_vlan_grp *te_vlan = NULL;
  struct nsm_msg_pbb_te_vlan pbb_te_vlan_msg;
  struct pbb_te_vid *pbb_vid;
  struct avl_node *te_node = NULL;
  struct avl_node *pbb_vid_node = NULL;
#if defined HAVE_I_BEB && defined HAVE_B_BEB
  struct avl_tree *pbb_te_inst_tbl = NULL;
  struct pbb_te_inst_entry * pbb_te_entry = NULL;
  struct pbb_te_esp_entry *esp = NULL;
  struct interface *pnp_ifp = NULL;
  struct nsm_msg_pbb_te_esp pbb_te_esp_msg;
  struct nsm_msg_pbb_esp_pnp pbb_esp_pnp_msg;
  struct avl_node *pbb_te_entry_node = NULL;
  struct avl_node *esp_node = NULL;
 struct avl_node *ifp_node = NULL;
#endif

#endif /*HAVE_PBB_TE */

  if (! nm || ! nse || ! nm->bridge)
    return -1;

  master = nsm_bridge_get_master (nm);

  if (! master)
    return -1;

#ifdef HAVE_I_BEB
  bridge = master->bridge_list;

  if (bridge == NULL)
    {
      return -1;
    }

  while (bridge)
    {
      if (bridge->backbone_edge == 1)
        {
          /* This bridge can only be a i-comp bridge and not a b-comp */
          icomp = bridge->master->beb->icomp;

          if (icomp == NULL)
            return -1;

          AVL_TREE_LOOP (icomp->cnp_srv_tbl_list, cnp_node, node)
            {
              LIST_LOOP (cnp_node->svlan_bundle_list, svlan_bdl, svlan_node)
                {
#ifdef HAVE_B_BEB
                  /* This check is to ensure that Logical interface is sent
                   * only once for each cnp_node */
                  if (logical_ifp == 0)
                    {
                      ifindex_pip = NSM_LOGICAL_INTERFACE_INDEX + bridge->bridge_id;
                      ifindex_cbp = NSM_LOGICAL_INTERFACE_INDEX - bridge->bridge_id;

                      AVL_TREE_LOOP(bridge->port_tree, br_port, br_node)
                        {
                          if (br_port->nsmif->ifp->ifindex == ifindex_pip)
                            {
                              ifp_pip = br_port->nsmif->ifp;
                              break;
                            }
                          else
                            continue;
                        }

                      AVL_TREE_LOOP(master->b_bridge->port_tree, br_port, br_node)
                        {
                          if (br_port->nsmif->ifp->ifindex == ifindex_cbp)
                            {
                              ifp_cbp = br_port->nsmif->ifp;
                              break;
                            }
                          else
                            continue;
                        }


                      if (ifp_pip && ifp_cbp)
                        {
                          pbb_send_logical_interface_event (bridge->name, ifp_pip, PAL_TRUE);
                          pbb_send_logical_interface_event ("backbone", ifp_cbp, PAL_TRUE );

                          /* Send port type message to PM. */
                          pal_mem_set(&port_type_msg,0,sizeof(port_type_msg));

                          port_type_msg.ifindex = ifp_pip->ifindex;
                          port_type_msg.port_type = NSM_VLAN_PORT_MODE_PIP;
                          nsm_vlan_send_port_type_msg (bridge, &port_type_msg);

                          pal_mem_set(&port_type_msg,0,sizeof(port_type_msg));
                          port_type_msg.ifindex = ifp_cbp->ifindex;
                          port_type_msg.port_type = NSM_VLAN_PORT_MODE_CBP;
                          nsm_vlan_send_port_type_msg (bridge, &port_type_msg);

                          /* send nsm link update message to cfm */
                          NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_HW_ADDRESS);
                          /* Announce to the PMs.  */
                          nsm_server_if_update (ifp_pip, cindex);
                          nsm_server_if_update (ifp_cbp, cindex);

                        }
                      logical_ifp = 1;
                    }

#endif /* HAVE_B_BEB */
                  /* For each instance of svlan bundle a message has to be
                   * sent to the Protocol Modules */
                  pal_mem_cpy(&isid_to_pip.bridge_name, bridge->name, NSM_BRIDGE_NAMSIZ);
                  isid_to_pip.cnp_ifindex       = cnp_node->key.icomp_port_num;
                  isid_to_pip.pip_ifindex       = cnp_node->ref_port_num;
                  isid_to_pip.svid_low          = svlan_bdl->svid_low;
                  isid_to_pip.svid_high         = svlan_bdl->svid_high;
                  isid_to_pip.isid              = cnp_node->key.sid;
                  isid_to_pip.dispatch_status   = 0;

                  /* Send the message */
                  nsm_pbb_send_isid2svid(bridge, &isid_to_pip, NSM_MSG_PBB_ISID_TO_PIP_ADD);
                }/* LIST_LOOP (cnp_node->svlan_bundle_list, svlan_bdl, svlan_node) */

            }/* AVL_TREE_LOOP (icomp->cnp_srv_tbl_list, cnp_node, node) */
#ifdef HAVE_B_BEB
          logical_ifp = 0;
#endif /* HAVE_B_BEB */
        }/* if (bridge->backbone_edge) */
#ifdef HAVE_PBB_TE
  /* For mapping of TE-VLANS to TE-SID update*/
  if (bridge->pbb_te_group_tree)
  AVL_TREE_LOOP (bridge->pbb_te_group_tree, te_vlan, te_node)
    {
      if (te_vlan->vlan_tree)
        {
          AVL_TREE_LOOP(te_vlan->vlan_tree, pbb_vid, pbb_vid_node)
            {
              pbb_te_vlan_msg.tesid = te_vlan->tesid;
              pbb_te_vlan_msg.vid = pbb_vid->esp_vid;
              pal_strncpy (pbb_te_vlan_msg.brname, bridge->name, NSM_BRIDGE_NAMSIZ);
               nsm_pbb_send_te_vlan (bridge, &pbb_te_vlan_msg, NSM_MSG_PBB_TE_VID_ADD);
            }
        }
      else
        {
          pbb_te_vlan_msg.tesid = te_vlan->tesid;
          pbb_te_vlan_msg.vid = 0;
          pal_strncpy (pbb_te_vlan_msg.brname, bridge->name, NSM_BRIDGE_NAMSIZ);
          nsm_pbb_send_te_vlan (bridge, &pbb_te_vlan_msg, NSM_MSG_PBB_TE_VID_ADD);

        }
    }
#endif /* HAVE_PBB_TE */
      /* Next bridge. */
      bridge = bridge->next;

    }/* while (bridge) */
#endif /* HAVE_I_BEB */

#ifdef HAVE_B_BEB
  node = NULL;
  bridge = master->b_bridge;

  if (bridge == NULL)
    return -1;

  if (pal_strncmp (bridge->name, "backbone", 8))
    return -1;

  bcomp =  bridge->master->beb->bcomp;

  if (bcomp == NULL)
    return -1;

  AVL_TREE_LOOP (bcomp->cbp_srvinst_list, cbp_node, node)
    {
      /* For each instance of svlan bundle a message has to be
       * sent to the Protocol Modules */
      isid_to_bvid.cbp_ifindex       = cbp_node->key.bcomp_port_num;
      isid_to_bvid.isid              = cbp_node->key.b_sid;
      isid_to_bvid.bvid              = cbp_node->bvid;
      isid_to_bvid.dispatch_status   = 0;

      nsm_pbb_send_isid2bvid (bridge, &isid_to_bvid, NSM_MSG_PBB_ISID_TO_BVID_ADD);
    }

#ifdef HAVE_PBB_TE
  /* For mapping of TE-VLANS to TE-SID update*/
  if (bridge->pbb_te_group_tree)
  AVL_TREE_LOOP (bridge->pbb_te_group_tree, te_vlan, te_node)
    {
      if (te_vlan->vlan_tree)
        {
          AVL_TREE_LOOP(te_vlan->vlan_tree, pbb_vid, pbb_vid_node)
            {
              pbb_te_vlan_msg.tesid = te_vlan->tesid;
              pbb_te_vlan_msg.vid = pbb_vid->esp_vid;
              pal_strncpy (pbb_te_vlan_msg.brname, bridge->name, NSM_BRIDGE_NAMSIZ);
               nsm_pbb_send_te_vlan (bridge, &pbb_te_vlan_msg, NSM_MSG_PBB_TE_VID_ADD);
            }
        }
    }
#if defined HAVE_I_BEB && defined HAVE_B_BEB
 if ((bcomp != NULL) && (bcomp->pbb_te_inst_table))
   pbb_te_inst_tbl = bcomp->pbb_te_inst_table;

 if (pbb_te_inst_tbl)
   AVL_TREE_LOOP (pbb_te_inst_tbl, pbb_te_entry, pbb_te_entry_node)
     {
       AVL_TREE_LOOP ( pbb_te_entry->esp_comp_list, esp, esp_node)
         {
           pbb_te_esp_msg.tesid = esp->te_sid;
           pbb_te_esp_msg.esp_index = 0;
           pbb_te_esp_msg.cbp_ifindex = esp->cbp_ifp->ifindex;
           pal_mem_cpy (pbb_te_esp_msg.remote_mac, esp->remote_mac,
               ETHER_ADDR_LEN);
           pal_mem_cpy (pbb_te_esp_msg.mcast_rx_mac, esp->mcast_rx_mac,
               ETHER_ADDR_LEN);
           pbb_te_esp_msg.esp_vid = esp->esp_vid;
           pbb_te_esp_msg.multicast = esp->multicast;
           pbb_te_esp_msg.egress = esp->egress;

           nsm_pbb_send_te_esp (bridge, &pbb_te_esp_msg, NSM_MSG_PBB_TE_ESP_ADD);

         }
     }

 if (pbb_te_inst_tbl)
   AVL_TREE_LOOP (pbb_te_inst_tbl, pbb_te_entry, pbb_te_entry_node)
     {
       if (pbb_te_entry && pbb_te_entry->esp_comp_list)
         esp = nsm_pbb_te_search_egress_esp (pbb_te_entry->esp_comp_list);

       if (esp  && esp->tree_pnp_ifp)
         {
           AVL_TREE_LOOP (esp->tree_pnp_ifp, pnp_ifp, ifp_node)
             {
               pal_mem_set (&pbb_esp_pnp_msg, 0, sizeof (struct nsm_msg_pbb_esp_pnp));
               pbb_esp_pnp_msg.tesid = pbb_te_entry->te_sid;
               pbb_esp_pnp_msg.pnp_ifindex = pnp_ifp->ifindex;
               pbb_esp_pnp_msg.cbp_ifindex = pbb_te_entry->cbp_ifp->ifindex;
               nsm_pbb_send_esp_pnp (&pbb_esp_pnp_msg, NSM_MSG_PBB_TE_ESP_PNP_ADD);

             }

         }
     }

#endif /* HAVE_B_BEB && HAVE_I_BEB */
#endif /*HAVE_PBB_TE*/

#endif /* HAVE_B_BEB */

  return 0;
}
#endif /* HAVE_I_BEB || HAVE_B_BEB */

#ifdef HAVE_ELMID

s_int32_t
nsm_elmi_bw_sync (struct nsm_bridge_port *br_port, struct interface *ifp)
{
  struct nsm_if *zif = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_band_width_profile *bw_profile = NULL;
  struct nsm_uni_evc_bw_profile *evc_profile = NULL;
  struct nsm_bw_profile_parameters *ingress_evc = NULL;
  struct nsm_msg_bw_profile bw_msg;
  struct listnode *evc_node = NULL;

  zif = (struct nsm_if *)ifp->info;
  if (! zif)
    return RESULT_ERROR;

  if (zif->nsm_bw_profile)
    bw_profile = zif->nsm_bw_profile;

  vlan_port = &br_port->vlan_port;

  if (!vlan_port || (vlan_port->mode != NSM_VLAN_PORT_MODE_CE))
    return RESULT_ERROR;

  pal_mem_set (&bw_msg, 0, sizeof (struct nsm_msg_bw_profile));

  if (bw_profile)
    {
      /* Display Ingress Bandwidth Profile per UNI in show run*/
      if (bw_profile->ingress_uni_bw_profile)
        {
          if (bw_profile->ingress_uni_bw_profile->active == PAL_TRUE)
            {
              /* send msg to elmi regarding uni bw profile */
              NSM_SET_CTYPE(bw_msg.cindex, NSM_ELMI_CTYPE_UNI_BW);

              bw_msg.ifindex = br_port->ifindex;
              bw_msg.cir = bw_profile->ingress_uni_bw_profile->comm_info_rate;
              bw_msg.cbs = bw_profile->ingress_uni_bw_profile->comm_burst_size;
              bw_msg.eir = bw_profile->ingress_uni_bw_profile->excess_info_rate;
              bw_msg.ebs = bw_profile->ingress_uni_bw_profile->excess_burst_size;
              bw_msg.cf = bw_profile->ingress_uni_bw_profile->coupling_flag;
              bw_msg.cm = bw_profile->ingress_uni_bw_profile->color_mode;

              nsm_server_elmi_send_bw (br_port, &bw_msg, NSM_MSG_ELMI_UNI_BW_ADD);

            }
        }

      if (bw_profile->uni_evc_bw_profile_list)
        {
          LIST_LOOP (bw_profile->uni_evc_bw_profile_list, evc_profile,
              evc_node)
            {
              if ((evc_profile->evc_id) &&
                  (evc_profile->instance_id != PAL_FALSE))
                {

                  if (evc_profile->ingress_evc_bw_profile)
                    {
                      ingress_evc =
                        evc_profile->ingress_evc_bw_profile;

                      if (ingress_evc->active == PAL_TRUE)
                        {
                          /* send msg to elmi regarding evc bw profile */
                          NSM_SET_CTYPE(bw_msg.cindex, NSM_ELMI_CTYPE_EVC_BW);

                          bw_msg.ifindex = br_port->ifindex;
                          bw_msg.evc_ref_id = evc_profile->svlan;
                          bw_msg.cir = ingress_evc->comm_info_rate;
                          bw_msg.cbs = ingress_evc->comm_burst_size;
                          bw_msg.eir = ingress_evc->excess_info_rate;
                          bw_msg.ebs = ingress_evc->excess_burst_size;
                          bw_msg.cf = ingress_evc->coupling_flag;
                          bw_msg.cm = ingress_evc->color_mode;

                          nsm_server_elmi_send_bw (br_port, &bw_msg,
                              NSM_MSG_ELMI_EVC_BW_ADD);
                        }
                    }

                }

            }

        }
    }

  return RESULT_OK;
}

s_int32_t
nsm_elmi_sync (struct nsm_master *nm, struct nsm_server_entry *nse)
{

  struct nsm_bridge_master *master = NULL;
  struct nsm_bridge *bridge = NULL;
  struct avl_node *node = NULL;
  struct nsm_svlan_reg_info *svlan_info = NULL;
  struct nsm_cvlan_reg_tab *regtab = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct interface *ifp = NULL;
  struct nsm_if *zif = NULL;
  u_int16_t svid = 0;

  if (! nm || ! nse || ! nm->bridge)
    return RESULT_ERROR;

  master = nsm_bridge_get_master (nm);

  if (! master)
    return RESULT_ERROR;

  bridge = master->bridge_list;

  if (bridge == NULL)
    return RESULT_ERROR;

  while (bridge)
    {

      AVL_TREE_LOOP (bridge->port_tree, br_port, node)
        {
if (!((NSM_BRIDGE_TYPE_PROVIDER (bridge)
               && (bridge->provider_edge == PAL_TRUE))))
            continue;

          if ((zif = br_port->nsmif) == NULL)
            continue;

          if ((ifp = zif->ifp) == NULL)
            continue;

          vlan_port = &br_port->vlan_port;

          if (vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
            continue;

          /* Send uni info */
          nsm_elmi_send_uni_add (bridge, ifp);

          regtab = vlan_port->regtab;
          if (regtab == NULL)
            continue;

          NSM_VLAN_SET_BMP_ITER_BEGIN (vlan_port->svlanMemberBmp, svid)
            {
              if ((svlan_info = nsm_reg_tab_svlan_info_lookup (regtab,
                      svid)) != NULL)
                nsm_elmi_send_evc_add (bridge, ifp, svid, svlan_info);
            }
          NSM_VLAN_SET_BMP_ITER_END (vlan_port->svlanMemberBmp, svid);

          nsm_elmi_bw_sync (br_port, ifp);

        }
      bridge = bridge->next;
    }
  return RESULT_OK;

}

#endif /* HAVE_ELMID */

