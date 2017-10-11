/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "if.h"
#include "prefix.h"
#include "thread.h"
#include "linklist.h"
#include "log.h"
#include "checksum.h"

#include "ospfd/ospfd.h"
#include "ospfd/ospf_interface.h"
#include "ospfd/ospf_vlink.h"
#ifdef HAVE_OSPF_MULTI_AREA
#include "ospfd/ospf_multi_area_link.h"
#endif /* HAVE_OSPF_MULTI_AREA */
#include "ospfd/ospf_network.h"
#include "ospfd/ospf_packet.h"
#include "ospfd/ospf_neighbor.h"
#include "ospfd/ospf_ifsm.h"
#include "ospfd/ospf_nfsm.h"
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_lsdb.h"
#include "ospfd/ospf_flood.h"
#include "ospfd/ospf_abr.h"
#include "ospfd/ospf_asbr.h"
#include "ospfd/ospf_api.h"
#include "ospfd/ospf_vrf.h"
#ifdef HAVE_SNMP
#include "ospfd/ospf_snmp.h"
#endif /* HAVE_SNMP */
#ifdef HAVE_RESTART
#include "ospfd/ospf_restart.h"
#endif /* HAVE_RESTART */
#include "ospfd/ospf_debug.h"


struct ospf_packet *
ospf_packet_new (size_t size)
{
  struct ospf_packet *new;
  size_t length;

  if (size < OSPF_PACKET_MIN_SIZE)
    size = OSPF_PACKET_MIN_SIZE;

  length = sizeof (struct ospf_packet) + size - OSPF_HEADER_SIZE;
  new = XMALLOC (MTYPE_OSPF_PACKET, length);
  if (new == NULL)
    return NULL;

  /* Reset the packet members.  */
  new->prev = NULL;
  new->next = NULL;
  new->flags = 0;
  new->type = 0;
  new->dummy = 0;
  new->length = 0;
  new->alloced = size;
  new->dst.s_addr = 0;

  return new;
}

void
ospf_packet_free (struct ospf_packet *op)
{
  XFREE (MTYPE_OSPF_PACKET, op);
}


/* Unused OSPF packet handling.  */
int
ospf_packet_delete_unuse_tail (struct ospf *top)
{
  struct ospf_packet *op;

  op = top->op_unuse_tail;
  if (op == NULL)
    return 0;

  top->op_unuse_tail = op->prev;

  if (top->op_unuse_tail != NULL)
    top->op_unuse_tail->next = NULL;
  else
    top->op_unuse_head = NULL;

  /* Free the patcket.  */
  ospf_packet_free (op);

  top->op_unuse_count--;

  return 1;
}

void
ospf_packet_add_unuse (struct ospf *top, struct ospf_packet *op)
{
  /* Free the packet if the unused packet handling is disabled.  */
  if (top->op_unuse_max == 0)
    {
      ospf_packet_free (op);
      return;
    }

  /* Reset the packet members.  */
  op->flags = 0;
  op->type = 0;
  op->length = 0;
  op->dst.s_addr = 0;
  op->prev = NULL;
  op->next = top->op_unuse_head;

  if (top->op_unuse_head != NULL)
    top->op_unuse_head->prev = op;
  else
    top->op_unuse_tail = op;
  top->op_unuse_head = op;

  top->op_unuse_count++;

  /* Delete the last LSA when it exceeds maximum number.  */
  if (top->op_unuse_count > top->op_unuse_max)
    ospf_packet_delete_unuse_tail (top);
}

struct ospf_packet *
ospf_packet_delete_unuse (struct ospf *top, struct ospf_packet *op)
{
  if (op->prev != NULL)
    op->prev->next = op->next;
  else
    top->op_unuse_head = op->next;

  if (op->next != NULL)
    op->next->prev = op->prev;
  else
    top->op_unuse_tail = op->prev;

  op->prev = NULL;
  op->next = NULL;

  top->op_unuse_count--;

  return op;
}

struct ospf_packet *
ospf_packet_lookup_unuse (struct ospf *top, size_t size)
{
  struct ospf_packet *op;

  for (op = top->op_unuse_head; op; op = op->next)
    if (op->alloced >= size)
      return ospf_packet_delete_unuse (top, op);

  /* Free the last unused packet to avoid
     the excessive allocation.  */
  ospf_packet_delete_unuse_tail (top);

  /* Returns the new one.  */
  return ospf_packet_new (size);
}

void
ospf_packet_unuse_clear (struct ospf *top)
{
  while (ospf_packet_delete_unuse_tail (top))
    ;
}

void
ospf_packet_unuse_clear_half (struct ospf *top)
{
  u_int32_t half;

  /* Free half of them.  */
  half = top->op_unuse_count / 2;

  while (ospf_packet_delete_unuse_tail (top))
    if (top->op_unuse_count <= half)
      return;
}

void
ospf_packet_unuse_max_update (struct ospf *top, u_int32_t max)
{
  if (top->op_unuse_max != max)
    {
      while (top->op_unuse_count > max)
        if (!ospf_packet_delete_unuse_tail (top))
          break;

      /* Update the maximum number.  */
      top->op_unuse_max = max;
    }
}


/* OSPF packet FIFO handling.  */
void
ospf_packet_add (struct ospf_interface *oi, struct ospf_packet *op)
{
  op->prev = oi->op_tail;
  op->next = NULL;

  if (oi->op_tail != NULL)
    oi->op_tail->next = op;
  else
    oi->op_head = op;
  oi->op_tail = op;
}

void
ospf_packet_delete (struct ospf_interface *oi, struct ospf_packet *op)
{
  if (op->next != NULL)
    op->next->prev = op->prev;
  else
    oi->op_tail = op->prev;

  if (op->prev != NULL)
    op->prev->next = op->next;
  else
    oi->op_head = op->next;

  ospf_packet_add_unuse (oi->top, op);
}

struct ospf_packet *
ospf_packet_get (struct ospf_interface *oi,
                 u_char type, struct pal_in4_addr *dst)
{
  struct ospf_master *om = oi->top->om;
  u_char auth_type = ospf_auth_type (oi);
  struct ospf *top = oi->top;
  struct stream *s = top->obuf;
  struct ospf_packet *op;
  size_t length;
  size_t size;

  length = size = stream_get_endp (s);
  if (auth_type == OSPF_AUTH_CRYPTOGRAPHIC)
    size += OSPF_AUTH_MD5_SIZE;
#ifdef HAVE_RESTART
  if (type == OSPF_PACKET_HELLO || type == OSPF_PACKET_DB_DESC)
    if (top->restart_method == OSPF_RESTART_METHOD_SIGNALING)
      size += ospf_restart_lls_size_get (oi);
#endif /* HAVE_RESTART */

  /* Packet size check.  */
  if (size > OSPF_PACKET_MAX_SIZE)
    {
      if (IS_DEBUG_OSPF_PACKET (type, SEND))
        zlog_warn (ZG, "SEND[%s]: Packet exhaust MAX packet size",
                   LOOKUP (ospf_packet_type_msg, type));
      return NULL;
    }

  /* Lookup the OSPF packet buffer.  */
  op = ospf_packet_lookup_unuse (top, size);
  if (op == NULL)
    {
      if (IS_DEBUG_OSPF_PACKET (type, SEND))
        zlog_warn (ZG, "SEND[%s]: Can't allocate OSPF packet buffer",
                   LOOKUP (ospf_packet_type_msg, type));
      return NULL;
    }

  op->type = type;
  if (auth_type == OSPF_AUTH_CRYPTOGRAPHIC)
    SET_FLAG (op->flags, OSPF_PACKET_FLAG_MD5);
#ifdef HAVE_RESTART
  if (type == OSPF_PACKET_HELLO || type == OSPF_PACKET_DB_DESC)
    if (top->restart_method == OSPF_RESTART_METHOD_SIGNALING)
      SET_FLAG (op->flags, OSPF_PACKET_FLAG_LLS);
#endif /* HAVE_RESTART */
  op->length = length;
  op->dst = *dst;

  pal_mem_cpy (op->buf, STREAM_DATA (s), op->length);

  return op;
}

struct ospf_packet *
ospf_packet_dup (struct ospf_interface *oi,
                 struct ospf_packet *op, struct pal_in4_addr *dst)
{
  struct ospf_master *om = oi->top->om;
  struct ospf_packet *new;

  /* Lookup the OSPF packet buffer.  */
  new = ospf_packet_lookup_unuse (oi->top, op->alloced);
  if (new == NULL)
    {
      if (IS_DEBUG_OSPF_PACKET (op->type, SEND))
        zlog_warn (ZG, "SEND[%s]: Can't allocate OSPF packet buffer",
                   LOOKUP (ospf_packet_type_msg, op->type));
      return NULL;
    }

  new->type = op->type;
  new->flags = op->flags;
  new->length = op->length;
  new->dst = *dst;

  pal_mem_cpy (new->buf, op->buf, op->length);

  return new;
}

#define OSPF_PACKET_OFFSET                                                    \
    (sizeof (struct pal_in4_header))

int
ospf_packet_max (struct ospf_interface *oi)
{
  int max;

  max = ospf_if_mtu_get (oi) - OSPF_PACKET_OFFSET;
  if (ospf_auth_type (oi) == OSPF_AUTH_CRYPTOGRAPHIC)
    max -= OSPF_AUTH_MD5_SIZE;

  max -= 25;

  return max;
}


#ifdef HAVE_MD5
void
ospf_packet_md5_digest_calc (struct ospf_crypt_key *ck, void *ibuf,
                             u_int16_t length, u_char *digest)
{
  MD5_CTX ctx;

  /* Generate a digest for the ospf packet - their digest + our digest. */
  md5_init_ctx (&ctx);
  md5_process_bytes (ibuf, length, &ctx);
  md5_process_bytes ((ck != NULL ?
                      (u_char *)ck->auth_key : OSPFMessageDigestKeyUnspec),
                     OSPF_AUTH_MD5_SIZE, &ctx);
  md5_finish_ctx (&ctx, digest);
}

int
ospf_packet_md5_digest_check (struct ospf_interface *oi, u_int16_t length)
{
  void *ibuf = STREAM_DATA (oi->top->ibuf);
  struct ospf_header *ospfh = (struct ospf_header *)ibuf;
  struct ospf_neighbor *nbr;
  struct ospf_crypt_key *ck;
  u_char digest[OSPF_AUTH_MD5_SIZE];
  u_char *pdigest;

  /* Get pointer to the end of the packet. */
  pdigest = (u_char *)ibuf + length;

  /* check crypto seqnum. */
  nbr = ospf_nbr_lookup_by_router_id (oi->nbrs, &ospfh->router_id);
  if (nbr != NULL
      && nbr->crypt_seqnum > pal_ntoh32 (ospfh->u.crypt.crypt_seqnum))
    return 0;

  /* Get secret key. */
  ck = ospf_if_message_digest_key_get (oi, ospfh->u.crypt.key_id);

  /* Generate a digest for the ospf packet - their digest + our digest. */
  ospf_packet_md5_digest_calc (ck, ibuf, length, digest);

  /* Compare the two digests. */
  if (pal_mem_cmp (pdigest, digest, OSPF_AUTH_MD5_SIZE))
    return 0;

  return 1;
}

/* This function is called from ospf_write(), it will detect the
   authentication scheme and if it is MD5, it will change the sequence
   and update the MD5 digest. */
int
ospf_packet_md5_digest_set_by_keyid (struct ospf_interface *oi, 
                                     struct ospf_packet *op, u_char keyid)
{
  struct ospf_header *ospfh;
  struct ospf_crypt_key *ck = NULL;
  u_char digest[OSPF_AUTH_MD5_SIZE];
  u_int16_t length;
  void *ibuf = op->buf;

  ospfh = (struct ospf_header *) ibuf;
  length = pal_ntoh16 (ospfh->length);

  /* Get MD5 Authentication key from auth_key list. */
  ck = ospf_if_message_digest_key_get (oi, keyid); 
  if (ck)
    {
      ospfh->u.crypt.zero = 0;
      ospfh->u.crypt.key_id = ck->key_id;
      ospfh->u.crypt.auth_data_len = OSPF_AUTH_MD5_SIZE;
    }

  /* Set Crypt Sequence Number. */
  oi->crypt_seqnum ++;
  OSPF_PNT_UINT32_PUT ((u_char *)&ospfh->u.crypt.crypt_seqnum,
                       0, oi->crypt_seqnum);

  /* Generate a digest for the entire packet + our secret key. */
  ospf_packet_md5_digest_calc (ck, ibuf, length, digest);

  /* Append md5 digest to the end of the stream. */
  pal_mem_cpy ((u_char *)ibuf + pal_ntoh16 (ospfh->length),
               digest, OSPF_AUTH_MD5_SIZE);

  /* We do *NOT* increment the OSPF header length. */
  op->length += OSPF_AUTH_MD5_SIZE;

  return OSPF_AUTH_MD5_SIZE;
}
#endif /* HAVE_MD5 */

u_int16_t
ospf_packet_auth_type (struct ospf_header *ospfh)
{
#ifdef HAVE_OSPF_MULTI_INST
 /* As auth_type is of 8-bit, no need to perform pal_ntoh16 */
  return (ospfh->auth_type);
#else
  return (pal_ntoh16(ospfh->auth_type));
#endif /* HAVE_OSPF_MULTI_INST */
}

/* Make Authentication Data. */
int
ospf_packet_auth_set (struct ospf_interface *oi, struct ospf_header *ospfh)
{
  struct ospf_crypt_key *ck;
  u_int16_t auth_type;
  
  /* Get the exact auth type */
  auth_type = ospf_packet_auth_type (ospfh);
  
  switch (auth_type)
    {
    case OSPF_AUTH_NULL:
      break;
    case OSPF_AUTH_SIMPLE:
      pal_strncpy (ospfh->u.auth_data, ospf_if_authentication_key_get (oi),
                   OSPF_AUTH_SIMPLE_SIZE);
      break;
    case OSPF_AUTH_CRYPTOGRAPHIC:
      ck = ospf_if_message_digest_key_get_last (oi);
      ospfh->u.crypt.zero = 0;
      ospfh->u.crypt.key_id = ck->key_id;
      ospfh->u.crypt.auth_data_len = OSPF_AUTH_MD5_SIZE;

      /* note: the seq is done in ospf_packet_md5_digest_set() */
      break;
    default:
      break;
    }

  return 0;
}


#ifdef HAVE_IN_PKTINFO
void
ospf_packet_msg_init (struct pal_msghdr *msg,
                      struct ospf_interface *oi, struct ospf_packet *op)
{
  static struct pal_sockaddr_in4 sin4;
  static struct pal_iovec iov;
  struct pal_in4_addr *src;

  /* Use the outgoing interface for the VLINK.  */
  if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
    oi = oi->u.vlink->out_oi;

  /* Get the source address.  */
  src = &oi->ident.address->u.prefix4;

  /* Initialize the message header.  */
  pal_sock_in4_cmsg_init (msg, PAL_CMSG_IPV4_PKTINFO);

  /* Set the packet.  */
  iov.iov_base = (void *)op->buf;
  iov.iov_len = op->length;

  /* Set the source address.  */
  pal_sock_in4_cmsg_pktinfo_set (msg, NULL, src, oi->u.ifp->ifindex);

  /* Set the destination address.  */
  pal_mem_set (&sin4, 0, sizeof (struct pal_sockaddr_in4));
  sin4.sin_family = AF_INET;
  sin4.sin_addr = op->dst;

  /* Prepare the message header.  */
  msg->msg_name = &sin4;
  msg->msg_namelen = sizeof (struct pal_sockaddr_in4);
  msg->msg_iov = &iov;
  msg->msg_iovlen = 1;
}
#else
void
ospf_packet_msg_init (struct pal_msghdr *msg,
                      struct ospf_interface *oi, struct ospf_packet *op)
{
  static struct pal_sockaddr_in4 sin4;
  static struct pal_in4_header iph;
  static struct pal_iovec iov[2];
  u_char ttl;

  /* Use the outgoing interface for the VLINK.  */
  if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
    {
      oi = oi->u.vlink->out_oi;
      ttl = OSPF_VLINK_IP_TTL;
    }
  else
    ttl = OSPF_IP_TTL;

  /* Initialize the message header.  */
  pal_sock_in4_cmsg_init (msg, 0);

  /* Prepare the IP header.  */
  pal_in4_ip_hdr_len_set (&iph, 0);
  iph.ip_v = IPVERSION;
#ifdef IPTOS_PREC_INTERNETCONTROL
  iph.ip_tos = IPTOS_PREC_INTERNETCONTROL;
#endif /* IPTOS_PREC_INTERNETCONTROL */
  pal_in4_ip_packet_len_set (&iph, op->length);
  iph.ip_id = 0;
  iph.ip_off = 0;
  iph.ip_ttl = ttl;
  iph.ip_p = IPPROTO_OSPFIGP;
  iph.ip_sum = 0;
  iph.ip_src = oi->ident.address->u.prefix4;
  iph.ip_dst = op->dst;

  /* Set the packet.  */
  iov[0].iov_base = (void *)&iph;
  iov[0].iov_len = IP_HEADER_SIZE;
  iov[1].iov_base = (void *)op->buf;
  iov[1].iov_len = op->length;

  /* Set the destination address.  */
  pal_mem_set (&sin4, 0, sizeof (struct pal_sockaddr_in4));
  sin4.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
  sin4.sin_len = sizeof (struct pal_sockaddr_in4);
#endif /* HAVE_SIN_LEN */
  sin4.sin_addr = op->dst;

  /* Prepare the message header.  */
  msg->msg_name = &sin4;
  msg->msg_namelen = sizeof (struct pal_sockaddr_in4);
  msg->msg_iov = iov;
  msg->msg_iovlen = 2;
}
#endif /* HAVE_IN_PKTINFO */

void
ospf_packet_msg_finish (struct pal_msghdr *msg)
{
  pal_sock_in4_cmsg_finish (msg);
}

void
ospf_packet_send (struct ospf_interface *oi, struct ospf_packet *op)
{
  char if_str[OSPF_IF_STRING_MAXLEN];
  struct ospf_master *om = oi->top->om;
  struct pal_msghdr msg;
  int flags = 0;
  int ret;

  /* Initialize message header.  */
  ospf_packet_msg_init (&msg, oi, op);

  if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
    oi = oi->u.vlink->out_oi;

  /* NetBSD can't handle MSG_DONTROUTE for multicast addresses.  */
  else if (!IN_MULTICAST (pal_ntoh32 (op->dst.s_addr)))
    flags = MSG_DONTROUTE;

  /* Debug the packet.  */
  if (IS_DEBUG_OSPF_PACKET (op->type, SEND))
    {
      zlog_info (ZG, "SEND[%s]: To %r via %s, length %hu",
                 LOOKUP (ospf_packet_type_msg, op->type),
                 &op->dst, IF_NAME (oi), op->length);

      if (IS_DEBUG_OSPF_PACKET (op->type, DETAIL))
        ospf_packet_dump (om, op->buf);
    }

  /* Send the packet.  */
  ret = pal_sock_sendmsg (oi->top->fd, &msg, flags);
  if (ret < 0)
    if (IS_DEBUG_OSPF_PACKET (op->type, SEND))
      zlog_warn (ZG, "SEND[%s]: sendmsg error: %s",
                 IF_NAME (oi), pal_strerror (errno));

  /* Finish message header.  */
  ospf_packet_msg_finish (&msg);
}

int
ospf_packet_write (struct ospf_interface *oi, struct ospf_packet *op)
{
#ifdef HAVE_MD5
  struct ospf_if_params *oip;
  struct ospf_crypt_key *ck;
  struct listnode *node;
  u_int16_t auth_type;
#endif /* HAVE_MD5 */
  struct ospf_header *ospfh;
  int count = 0;

  /* Sanity check.  */
  pal_assert (op->length >= OSPF_HEADER_SIZE);

  ospfh = (struct ospf_header *)op->buf;

  if (IPV4_ADDR_SAME (&op->dst, &IPv4AllSPFRouters)
      || IPV4_ADDR_SAME (&op->dst, &IPv4AllDRouters))
    if (!ospf_if_ipmulticast (oi))
      return 0;

#ifdef HAVE_RESTART
  /* Set LLS block. */
  if (CHECK_FLAG (op->flags, OSPF_PACKET_FLAG_LLS))
    ospf_restart_lls_block_set (oi, op);
#endif /* HAVE_RESTART */

#ifdef HAVE_MD5
  auth_type = ospf_packet_auth_type (ospfh);

  /* Rewrite the md5 signature & update the seq.  */
  if (auth_type == OSPF_AUTH_CRYPTOGRAPHIC)
    {
      if (OSPF_IF_PARAM_CHECK (oi->params, AUTH_CRYPT))
        oip = oi->params;
      else if (OSPF_IF_PARAM_CHECK (oi->params_default, AUTH_CRYPT))
        oip = oi->params_default;
      else
        oip = NULL;

      if (oip && oip->auth_crypt)
        {
          LIST_LOOP (oip->auth_crypt, ck, node)
            if (!CHECK_FLAG (ck->flags, OSPF_AUTH_MD5_KEY_PASSIVE))
              {
                ospf_packet_md5_digest_set_by_keyid (oi, op, ck->key_id);
                ospf_packet_send (oi, op);
                count++;
              }

          if (count > 0)
            return 0;
        }

      ospf_packet_md5_digest_set_by_keyid (oi, op, 0);
      ospf_packet_send (oi, op);
    }
  else
#endif /* HAVE_MD5 */
    {
      ospf_packet_auth_set (oi, ospfh);
      ospf_packet_send (oi, op);
    }
   
  return 0;
}

void
ospf_packet_clear_all (struct ospf_interface *oi)
{
  while (oi->op_head != NULL)
    ospf_packet_delete (oi, oi->op_head);
}

void
ospf_packet_send_all (struct ospf_interface *oi)
{
  while (oi->op_head != NULL)
    {
      ospf_packet_write (oi, oi->op_head);
      ospf_packet_delete (oi, oi->op_head);
    }
}

int
ospf_write (struct thread *thread)
{
  struct ospf *top = THREAD_ARG (thread);
  struct ospf_interface *oi;
  struct listnode *node;
  struct ospf_master *om = top->om;

  top->t_write = NULL;

  node = LISTHEAD (top->oi_write_q);
  pal_assert (node != NULL);
  oi = GETDATA (node);
  pal_assert (oi != NULL);
  pal_assert (CHECK_FLAG (oi->flags, OSPF_IF_WRITE_Q));

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  /* Write the packet to the socket.  */
  ospf_packet_write (oi, oi->op_head);

  /* Now delete the packet from the queue.  */
  ospf_packet_delete (oi, oi->op_head);

  /* Remove the interface from the interface write queue
     if the interface doesn't have any pending packets.  */
  if (oi->op_head == NULL)
    {
      UNSET_FLAG (oi->flags, OSPF_IF_WRITE_Q);
      list_delete_node (top->oi_write_q, node);
    }

  /* Register the next write thread if there is another interface
     that has the pending packets.  */
  if (!list_isempty (top->oi_write_q))
    OSPF_WRITE_ON (top);

  return 0;
}


/* Make OSPF header. */
void
ospf_packet_header_set (struct stream *s, struct ospf_interface *oi, int type)
{
  stream_putc (s, OSPF_VERSION);                /* Version #. */
  stream_putc (s, type);                        /* Type. */
  stream_putw (s, 0);                           /* Packet Length. */
  stream_put_in_addr (s, &oi->top->router_id);  /* Router ID. */
  stream_put_in_addr (s, &oi->area->area_id);   /* Area ID. */
  stream_putw (s, 0);                           /* Checksum. */
#ifdef HAVE_OSPF_MULTI_INST
  if (oi->network)
    {
      stream_putc (s, oi->network->inst_id);        /* Interface instance ID. */
      stream_putc (s, ospf_auth_type (oi));         /* Au type. */
    }
  else
    {
      stream_putc (s, OSPF_IF_INSTANCE_ID_DEFAULT);
      stream_putc (s, ospf_auth_type (oi));
    }
#else
  stream_putw (s, ospf_auth_type (oi));         /* AuType. */
#endif /* HAVE_OSPF_MULTI_INST */
  
  stream_put (s, 0, OSPF_AUTH_SIMPLE_SIZE);     /* Authentication. */
}

/* Fill rest of OSPF header. */
void
ospf_packet_header_complete (struct stream *s, struct ospf_interface *oi)
{
  struct ospf_header *ospfh;
  u_int16_t length;
  u_int16_t auth_type;

  length = stream_get_endp (s);

  /* Fill length. */
  ospfh = (struct ospf_header *) STREAM_DATA (s);
  ospfh->length = pal_hton16 (length);

  auth_type = ospf_packet_auth_type (ospfh);
  /* Calculate checksum. */
  if (auth_type != OSPF_AUTH_CRYPTOGRAPHIC)
    ospfh->checksum = in_checksum ((u_int16_t *)ospfh, length);
  else
    ospfh->checksum = 0;
}

#ifdef HAVE_NSSA
#define HELLO_OPTIONS_MASK  (OSPF_OPTION_E | OSPF_OPTION_NP)
#else /* HAVE_NSSA */
#define HELLO_OPTIONS_MASK  (OSPF_OPTION_E)
#endif /* HAVE_NSSA */

u_char
ospf_hello_options (struct ospf_interface *oi)
{
  struct ospf_area *area = oi->area;
  u_char options = 0;

  if (IS_AREA_DEFAULT (area))
    options = OSPF_OPTION_E;
#ifdef HAVE_NSSA
  else if (IS_AREA_NSSA (area))
    options = OSPF_OPTION_NP;
#endif /* HAVE_NSSA */

#ifdef HAVE_RESTART
  if (area->top->restart_method == OSPF_RESTART_METHOD_SIGNALING)
    options |= OSPF_OPTION_L;
#endif /* HAVE_RESTART */

  return options;
}

void
ospf_packet_hello_set (struct stream *s, struct ospf_interface *oi, int type)
{
  struct ospf_neighbor *nbr;
  struct ls_node *rn;
  struct pal_in4_addr mask;
  unsigned long p;
  int flag = 0;

  /* Set netmask of interface. */
  if (IS_OSPF_IPV4_UNNUMBERED (oi) || oi->type == OSPF_IFTYPE_VIRTUALLINK)
    pal_mem_set ((char *) &mask, 0, sizeof (struct pal_in4_addr));
  else
    masklen2ip (oi->ident.address->prefixlen, &mask);
  stream_put_ipv4 (s, mask.s_addr);

  /* Set Hello Interval. */
  stream_putw (s, ospf_if_hello_interval_get (oi));

  /* Set Options. */
  stream_putc (s, ospf_hello_options (oi));

  /* Set Router Priority. */
  stream_putc (s, oi->ident.priority);

  /* Set Router Dead Interval. */
  stream_putl (s, ospf_if_dead_interval_get (oi));

  /* In case force to reset neighbor state. */
  if (type != OSPF_HELLO_DEFAULT)
    {
      stream_put_ipv4 (s, 0);
      stream_put_ipv4 (s, 0);
    }

  /* Set Designated Router. */
  stream_put_ipv4 (s, oi->ident.d_router.s_addr);

  /* Preserver the pointer. */
  p = s->putp;

  /* Set Backup Designated Router. */
  stream_put_ipv4 (s, oi->ident.bd_router.s_addr);

  /* Set all neighbors. */
  for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
    if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
      if (nbr->ident.router_id.s_addr != 0)     /* Ignore 0.0.0.0 node. */
        if (nbr->state != NFSM_Attempt          /* Ignore Down neighbor. */
            && nbr->state != NFSM_Down)
          {
            ospf_buffer_stream_ensure (s, stream_get_endp (s) + 4);

            /* Check neighbor is sane? */
            if (nbr->ident.d_router.s_addr != 0
                && IPV4_ADDR_SAME (&nbr->ident.d_router,
                                   &oi->ident.address->u.prefix4)
                && IPV4_ADDR_SAME (&nbr->ident.bd_router,
                                   &oi->ident.address->u.prefix4))
              flag = 1;

            stream_put_ipv4 (s, nbr->ident.router_id.s_addr);
          }

  /* Let neighbor generate BackupSeen. */
  if (flag == 1)
    {
      stream_set_putp (s, p);
      stream_put_ipv4 (s, 0);
    }
}

u_char
ospf_db_desc_options (struct ospf_area *area)
{
  u_char options = 0;

  if (IS_AREA_DEFAULT (area))
    options |= OSPF_OPTION_E;

#ifdef HAVE_OPAQUE_LSA
  if (IS_OPAQUE_CAPABLE (area->top))
    options |= OSPF_OPTION_O;
#endif /* HAVE_OPAQUE_LSA */

#ifdef HAVE_RESTART
  if (area->top->restart_method == OSPF_RESTART_METHOD_SIGNALING)
    options |= OSPF_OPTION_L;
#endif /* HAVE_RESTART */

  return options;
}

void
ospf_packet_db_desc_set (struct stream *s, struct ospf_interface *oi,
                         struct ospf_neighbor *nbr)
{
  struct ospf_lsa *lsa;
  struct ls_node *rn;
  struct ospf_lsdb *lsdb;
  int i;

  lsdb = nbr->db_sum;

  /* Set Interface MTU. */
  if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
    stream_putw (s, 0);
  else
    stream_putw (s, ospf_if_mtu_get (oi));

  /* Set Options. */
  stream_putc (s, ospf_db_desc_options (oi->area));

  /* Keep pointer to flags. */
  stream_putc (s, nbr->dd.flags);

  /* Set DD Sequence Number. */
  stream_putl (s, nbr->dd.seqnum);

  if (ospf_db_summary_isempty (nbr))
    {
      if (nbr->state >= NFSM_Exchange)
        {
          UNSET_FLAG (nbr->dd.flags, OSPF_DD_FLAG_M);

          /* Set DD flags again */
          stream_putc_at (s, 27, nbr->dd.flags);
        }
      return;
    }

  /* Describe LSA Header from Database Summary List. */
  for (i = OSPF_MIN_LSA; i < OSPF_MAX_LSA; i++)
    for (rn = ls_table_top (lsdb->type[i].db); rn; rn = ls_route_next (rn))
      if ((lsa = RN_INFO (rn, RNI_DEFAULT)) != NULL)
        {
          if (!CHECK_FLAG (lsa->flags, OSPF_LSA_DISCARD))
            {
              struct lsa_header *lsah;
              u_int16_t ls_age;

              /* DD packet overflows interface MTU. */
              if (stream_get_endp (s) +
                OSPF_LSA_HEADER_SIZE > ospf_packet_max (oi))
              return;

              ospf_buffer_stream_ensure (s, stream_get_endp (s) +
                                         OSPF_LSA_HEADER_SIZE);

              /* Keep pointer to LS age. */
              lsah = (struct lsa_header *) (STREAM_DATA (s) +
                                            stream_get_putp (s));

              /* Proceed stream pointer. */
              stream_put (s, lsa->data, OSPF_LSA_HEADER_SIZE);

              /* Set LS age. */
              ls_age = LS_AGE (lsa);
              lsah->ls_age = pal_hton16 (ls_age);
            }

          /* Remove LSA from DB summary list. */
          ospf_lsdb_delete (lsdb, lsa);
        }
}

void
ospf_packet_ls_req_set (struct stream *s, struct ospf_neighbor *nbr)
{
  struct ls_node *rn;
  struct ospf_ls_request *lsr;
  unsigned long delta = OSPF_LS_REQ_KEY_SIZE;

  for (rn = ls_table_top (nbr->ls_req); rn; rn = ls_route_next (rn))
    if ((lsr = RN_INFO (rn, RNI_DEFAULT)))
      {
        /* LS Request packet overflows interface MTU. */
        if (stream_get_endp (s) + delta > ospf_packet_max (nbr->oi))
          {
            ls_unlock_node (rn);
            return;
          }
        ospf_buffer_stream_ensure (s, stream_get_endp (s) +
                                   OSPF_LS_REQ_KEY_SIZE);

        stream_put (s, rn->p->prefix, OSPF_LS_REQ_KEY_SIZE);
        nbr->ls_req_last = lsr;
      }
}

int
ls_age_increment (struct ospf_lsa *lsa, int delay)
{
  int age;

  age = IS_LSA_MAXAGE (lsa) ? OSPF_LSA_MAXAGE : LS_AGE (lsa) + delay;

  return (age > OSPF_LSA_MAXAGE ? OSPF_LSA_MAXAGE : age);
}

#define OSPF_LS_UPD_NUM_LSAS_OFFSET     24

int 
ospf_packet_ls_upd_set (struct stream *s,
                        struct ospf_interface *oi, vector queue)
{
  struct ospf_master *om = oi->top->om;
  struct lsa_header *lsah;
  struct ospf_lsa *current;
  struct ospf_lsa *lsa;
  u_int16_t ls_age;
  u_int32_t tmplen;
  int count = 0;
  int flag;
  int len;
  int i;

  /* Just place holder. */
  stream_putl (s, 0);

  for (i = 0; i < vector_max (queue); i++)
    if ((lsa = vector_slot (queue, i)))
      {
        len = pal_ntoh16 (lsa->data->length);
        flag = 0;

        /* If lsa on the queue and current in LSDB are different,
           lsa is already discarded, so it is not necessary to send. */
        current = ospf_lsdb_lookup (lsa->lsdb, lsa);
        if (current != NULL)
          if (current != lsa)
            {
              vector_slot (queue, i) = NULL;
              ospf_lsa_unlock (lsa);
              continue;
            }

        /* Check LSA size. */
        tmplen = sizeof (struct pal_in4_header) + OSPF_HEADER_SIZE
          + OSPF_LS_UPD_MIN_SIZE + len;
        if (tmplen > OSPF_PACKET_MAX_SIZE)
          {
            if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_LS_UPD, SEND))
              zlog_warn (ZG, "SEND[LS-Upd]: Packet exhaust MAX packet size");

            /* Don't send this LSA. */
            vector_slot (queue, i) = NULL;
            ospf_lsa_unlock (lsa);
            break;
          }

        tmplen = stream_get_endp (s) + len;
        if (tmplen > ospf_packet_max (oi))
          {
            if (!count)
              flag = 1;
            else
              break;
          }
        ospf_buffer_stream_ensure (s, tmplen);

        /* Keep pointer to LS age. */
        lsah = (struct lsa_header *) (STREAM_DATA (s) + stream_get_putp (s));

        /* Put LSA to Link State Update. */
        stream_put (s, lsa->data, pal_ntoh16 (lsa->data->length));

        /* Set LS age.  Each hop must increment an lsa_age
           by transmit_delay of OSPF interface. */
        ls_age = ls_age_increment (lsa, ospf_if_transmit_delay_get (oi));
        lsah->ls_age = pal_hton16 (ls_age);

        count++;

        vector_slot (queue, i) = NULL;
        ospf_lsa_unlock (lsa);
        if (flag)
          break;
      }

  /* Now set #LSAs. */
  stream_putl_at (s, OSPF_LS_UPD_NUM_LSAS_OFFSET, count);

  return count; 
}


int
ospf_packet_ls_ack_set (struct stream *s,
                        struct ospf_interface *oi, vector queue)
{
  struct ospf_lsa *lsa;
  int i;

  for (i = 0; i < vector_max (queue); i++)
    if ((lsa = vector_slot (queue, i)))
      {
        if (stream_get_endp (s) + OSPF_LSA_HEADER_SIZE > ospf_packet_max (oi))
          return 1;

        ospf_buffer_stream_ensure (s, stream_get_endp (s) +
                                   OSPF_LSA_HEADER_SIZE);

        stream_put (s, lsa->data, OSPF_LSA_HEADER_SIZE);

        vector_slot (queue, i) = NULL;
        ospf_lsa_unlock (lsa);
      }

  return 0;
}


/* Send OSPF Hello. */
void
ospf_hello_send (struct ospf_interface *oi,
                 struct pal_in4_addr *dst, int type)
{
  struct stream *s = oi->top->obuf;
  struct ospf_packet *op;

  /* If this is passive interface, do not send OSPF Hello. */
  if (CHECK_FLAG (oi->flags, OSPF_IF_PASSIVE))
    return;

  /* Reset stream. */
  stream_reset (s);

  /* Build packet. */
  ospf_packet_header_set (s, oi, OSPF_PACKET_HELLO);    /* OSPF Header. */
  ospf_packet_hello_set (s, oi, type);                  /* Hello body. */
  ospf_packet_header_complete (s, oi);                  /* Fill header. */

  /* Get the neighbor destination address from the interface type
     if it's not specified.  */
  if (dst == NULL)
    dst = OSPF_PACKET_DST_HELLO (oi);

  /* Get the packet.  */
  op = ospf_packet_get (oi, OSPF_PACKET_HELLO, dst);
  if (op == NULL)
    return;

  /* Write the packet.  */
  ospf_packet_write (oi, op);
  oi->hello_out++;

  /* Delete the packet.  */
  ospf_packet_add_unuse (oi->top, op);

  SET_FLAG (oi->flags, OSPF_IF_DR_ELECTION_READY);
}

/* Periodical Hello for NBMA.
   For Down neighbor, Hello is send by Poll-Timer. */
void
ospf_hello_send_nbma (struct ospf_interface *oi, int type)
{
  struct stream *s = oi->top->obuf;
  struct ospf_neighbor *nbr;
  struct ospf_packet *op;
  struct ls_node *rn;

  /* If this is passive interface, do not send OSPF Hello. */
  if (CHECK_FLAG (oi->flags, OSPF_IF_PASSIVE))
    return;

  /* Reset stream. */
  stream_reset (s);

  /* Build packet. */
  ospf_packet_header_set (s, oi, OSPF_PACKET_HELLO);    /* OSPF Header. */
  ospf_packet_hello_set (s, oi, type);                  /* Hello body. */
  ospf_packet_header_complete (s, oi);                  /* Fill header. */

  /* Check each NBMA neighbor to send. */
  for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
    if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
      {
        /* RFC2328 9.5.1.  Sending Hello packets on NBMA networks

           If the router is eligible to become Designated Router, it
           must periodically send Hello Packets to all neighbors that
           are also eligible.  In addition, if the router is itself the
           Designated Router or Backup Designated Router, it must also
           send periodic Hello Packets to all other neighbors.  This
           means that any two eligible routers are always exchanging
           Hello Packets, which is necessary for the correct operation
           of the Designated Router election algorithm.  To minimize
           the number of Hello Packets sent, the number of eligible
           routers on an NBMA network should be kept small.

           If the router is not eligible to become Designated Router,
           it must periodically send Hello Packets to both the
           Designated Router and the Backup Designated Router (if they
           exist).  */

        /*When the Neighbour is down*/
        if (nbr->state == NFSM_Down)
           continue;

        if (oi->state == IFSM_DROther)
          {
            if (nbr->ident.priority == 0)
              continue;

            if (oi->ident.priority == 0
                && !IS_DECLARED_DR (&nbr->ident)
                && !IS_DECLARED_BACKUP (&nbr->ident))
              continue;
          }

        /* Get the packet.  */
        op = ospf_packet_get (oi, OSPF_PACKET_HELLO,
                              &nbr->ident.address->u.prefix4);
        if (op == NULL)
          return;

        /* Write the packet.  */
        ospf_packet_write (oi, op);
        oi->hello_out++;

        /* Delete the packet.  */
        ospf_packet_add_unuse (oi->top, op);
      }

  SET_FLAG (oi->flags, OSPF_IF_DR_ELECTION_READY);
}

int
ospf_hello_timer_interval (struct ospf_interface *oi)
{
  int interval = ospf_if_hello_interval_get (oi);
  int jitter = interval * OSPF_HELLO_INTERVAL_JITTER;

  /* The next interval will be interval +/- jitter.  */
  interval += oi->top->jitter_seed++ % (jitter * 2 + 1) - jitter;

  if (interval < OSPF_HELLO_INTERVAL_MIN)
    return OSPF_HELLO_INTERVAL_MIN;
  else if (interval > OSPF_HELLO_INTERVAL_MAX)
    return OSPF_HELLO_INTERVAL_MAX;
  else
    return interval;
}

int
ospf_hello_timer (struct thread *thread)
{
  struct ospf_interface *oi = THREAD_ARG (thread);
  struct ospf_master *om = oi->top->om;
  char if_str[OSPF_IF_STRING_MAXLEN];
#ifdef HAVE_RESTART
  struct ospf *top = oi->top;
#endif /* HAVE_RESTART */

  oi->t_hello = NULL;

  /* Set VR context */
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  if (IS_DEBUG_OSPF (ifsm, IFSM_TIMERS))
    zlog_info (ZG, "IFSM[%s]: Hello timer expire", IF_NAME (oi));

  /* Sending hello packet. */
  if (IS_OSPF_NETWORK_NBMA (oi))
    ospf_hello_send_nbma (oi, OSPF_HELLO_DEFAULT);
  else
    ospf_hello_send (oi, NULL, OSPF_HELLO_DEFAULT);

#ifdef HAVE_RESTART
  if (IS_RESTART_SIGNALING (oi->top) || IS_GRACEFUL_RESTART (oi->top))
    OSPF_TOP_TIMER_ON (oi->top->t_restart_state, ospf_restart_state_timer,
                       OSPF_RESTART_STATE_TIMER_INTERVAL);
#endif /* HAVE_RESTART */

  /* Hello timer set. */
  OSPF_IFSM_TIMER_ON (oi->t_hello, ospf_hello_timer,
                      ospf_hello_timer_interval (oi));

  return 0;
}

/* Hello send for NBMA. */
int
ospf_hello_poll_timer (struct thread *thread)
{
  struct ospf_nbr_static *nbr_static = THREAD_ARG (thread);
  struct ospf_interface *oi = nbr_static->nbr->oi;
  struct ospf_master *om = oi->top->om;

  nbr_static->t_poll = NULL;

  /* Set VR context */
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
    zlog_info (ZG, "ROUTER: poll-timer expire for neighbor[%r]",
               &nbr_static->addr);

  if (oi->type == OSPF_IFTYPE_NBMA)
    {
      if (nbr_static->nbr->state == NFSM_Down)
        if (oi->ident.priority != 0)
          if (nbr_static->priority != 0
              || oi->state == IFSM_DR
              || oi->state == IFSM_Backup)
            ospf_hello_send (oi, &nbr_static->addr, OSPF_HELLO_DEFAULT);
    }
  else if (oi->type == OSPF_IFTYPE_POINTOMULTIPOINT_NBMA
           && oi->state == IFSM_PointToPoint)
    {
      if (nbr_static->nbr->state == NFSM_Down)
        ospf_hello_send (oi, &nbr_static->addr, OSPF_HELLO_DEFAULT);
    }

  if (nbr_static->poll_interval > 0)
    OSPF_POLL_TIMER_ON (nbr_static->t_poll, ospf_hello_poll_timer,
                        nbr_static->poll_interval);

  return 0;
}

int
ospf_hello_reply_event (struct thread *thread)
{
  struct ospf_neighbor *nbr = THREAD_ARG (thread);
  struct ospf_interface *oi = nbr->oi;
  struct ospf_master *om = oi->top->om;
  char nbr_str[OSPF_NBR_STRING_MAXLEN];

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  if (IS_DEBUG_OSPF (nfsm, NFSM_TIMERS))
    zlog_info (ZG, "NFSM[%s]: Hello-reply event", NBR_NAME (nbr));

  /* Send back the Hello responce to the neighbor.  */
  ospf_hello_send (oi, &nbr->ident.address->u.prefix4, OSPF_HELLO_DEFAULT);

  return 0;
}


/* Send OSPF Database Description. */
void
ospf_db_desc_send (struct ospf_neighbor *nbr)
{
  struct ospf_interface *oi = nbr->oi;
  struct stream *s = oi->top->obuf;
  struct ospf_packet *op;

  /* Reset stream. */
  stream_reset (s);

  /* Build packet. */
  ospf_packet_header_set (s, oi, OSPF_PACKET_DB_DESC);  /* OSPF Header. */
  ospf_packet_db_desc_set (s, oi, nbr);                 /* DD body. */
  ospf_packet_header_complete (s, oi);                  /* Fill header. */

  /* Get the packet.  */
  op = ospf_packet_get (oi, OSPF_PACKET_DB_DESC, OSPF_PACKET_DST_BY_NBR (nbr));
  if (op == NULL)
    return;

  /* Add packet to the interface output queue. */
  ospf_packet_add (oi, op);
  oi->db_desc_out++;

  /* Hook thread to write packet. */
  OSPF_IFSM_WRITE_ON (oi);

  /* Remove old DD packet, then copy new one and keep in neighbor structure. */
  if (nbr->dd.sent.packet != NULL)
    ospf_packet_add_unuse (oi->top, nbr->dd.sent.packet);

  nbr->dd.sent.packet = ospf_packet_dup (oi, op, &op->dst);
  pal_time_tzcurrent (&nbr->dd.sent.ts, NULL);
}

/* Re-send Database Description. */
void
ospf_db_desc_resend (struct ospf_neighbor *nbr, bool_t snmp_flag)
{
  struct ospf_header *ospfh;
  struct ospf_packet *op;
#ifdef HAVE_SNMP
  struct lsa_header *lsah;
  int trap_cnt, len;
#endif /* HAVE_SNMP */
  u_int16_t mtu;

#ifdef HAVE_SNMP
  trap_cnt = 0;
  len = 0;
#endif /* HAVE_SNMP */

  /* Duplicate packet.  */
  op = ospf_packet_dup (nbr->oi, nbr->dd.sent.packet,
                        OSPF_PACKET_DST_BY_NBR (nbr));
  if (op == NULL)
    return;

  /* Make sure MTU as current. */
  if (nbr->oi->type == OSPF_IFTYPE_VIRTUALLINK)
    mtu = 0;
  else
    mtu = ospf_if_mtu_get (nbr->oi);

  OSPF_PNT_UINT16_PUT (op->buf, 24, mtu);

  ospfh = (struct ospf_header *) op->buf;

#ifdef HAVE_SNMP
  if (snmp_flag)
    {
      len = pal_hton16 (ospfh->length);
      if (len > (OSPF_HEADER_SIZE + OSPF_DB_DESC_MIN_SIZE))
        {
          lsah = (struct lsa_header *)(((u_char *)op->buf)
                  + OSPF_HEADER_SIZE + OSPF_DB_DESC_MIN_SIZE);

          for (len -= (OSPF_HEADER_SIZE + OSPF_DB_DESC_MIN_SIZE); len >= OSPF_LSA_HEADER_SIZE;
               len -= OSPF_LSA_HEADER_SIZE)
            {  
              /* Sending SNMP traps for OSPF_MAX_RETX_TRAPS (5) */
              trap_cnt++;  
              if (trap_cnt <= OSPF_MAX_RETX_TRAPS)
                {
                  if (nbr->oi->type == OSPF_IFTYPE_VIRTUALLINK)
                    ospfVirtIfTxRetransmit (nbr->oi->u.vlink, 
                                            OSPF_PACKET_DB_DESC, (int)lsah->type,
                                            &lsah->id, &lsah->adv_router);
                  else
                    { 	          
#ifdef HAVE_OSPF_MULTI_AREA
                       if (!CHECK_FLAG (nbr->oi->flags, OSPF_MULTI_AREA_IF))
#endif
                         ospfTxRetransmit (nbr, nbr->oi->area, OSPF_PACKET_DB_DESC,
                                          (int)lsah->type, &lsah->id,
                                          &lsah->adv_router);
                    }
                  lsah++;
                }
              else
                break;
            }
        }
    }
#endif /* HAVE_SNMP */

  /* Calculate checksum. */
  ospfh->checksum = 0;
  pal_mem_set (ospfh->u.auth_data, 0, OSPF_AUTH_SIMPLE_SIZE);

  if ((ospf_auth_type (nbr->oi)) != OSPF_AUTH_CRYPTOGRAPHIC)
    ospfh->checksum = in_checksum ((u_int16_t *)ospfh,
                                   pal_ntoh16 (ospfh->length));

  /* Set auth. */
  ospf_packet_auth_set (nbr->oi, ospfh);
  if ((ospf_auth_type (nbr->oi)) == OSPF_AUTH_CRYPTOGRAPHIC)
    SET_FLAG (op->flags, OSPF_PACKET_FLAG_MD5);
  else
    UNSET_FLAG (op->flags, OSPF_PACKET_FLAG_MD5);

  /* Add packet to the interface output queue. */
  ospf_packet_add (nbr->oi, op);
  nbr->oi->db_desc_out++;

  /* Hook thread to write packet. */
  OSPF_IFSM_WRITE_ON (nbr->oi);
}


int
ospf_ls_req_timer (struct thread *thread)
{
  struct ospf_neighbor *nbr = THREAD_ARG (thread);
  struct ospf_master *om = nbr->oi->top->om;
#ifdef HAVE_SNMP
  struct ls_node *rn;
  struct ospf_ls_request *lsr;
  u_int8_t count = 0;
#endif /* HAVE_SNMP */

  /* Set VR context */
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  nbr->t_ls_req = NULL;

  /* Send Link State Request. */
  if (ospf_ls_request_count (nbr))
    {
      ospf_ls_req_send (nbr);
#ifdef HAVE_SNMP
      for (rn = ls_table_top (nbr->ls_req); rn; rn = ls_route_next (rn))
        /* Sending SNMP traps for OSPF_MAX_RETX_TRAPS (5) */ 
        if ((lsr = RN_INFO (rn, RNI_DEFAULT)) && count++ < OSPF_MAX_RETX_TRAPS)
          {
            if (nbr->oi->type == OSPF_IFTYPE_VIRTUALLINK)
              ospfVirtIfTxRetransmit (nbr->oi->u.vlink, OSPF_PACKET_LS_REQ,
                                     (int)lsr->data.type, &lsr->data.id,
                                     &lsr->data.adv_router);
            else
              ospfTxRetransmit (nbr, nbr->oi->area, OSPF_PACKET_LS_REQ,
                               (int)lsr->data.type, &lsr->data.id,
                               &lsr->data.adv_router);
          }
#endif /* HAVE_SNMP */
    }

  /* Set Link State Request retransmission timer. */
  OSPF_NFSM_TIMER_ON (nbr->t_ls_req, ospf_ls_req_timer, nbr->v_ls_req);

  return 0;
}

void
ospf_ls_req_event (struct ospf_neighbor *nbr)
{
  OSPF_EVENT_OFF (nbr->t_ls_req);
  OSPF_EVENT_ON (nbr->t_ls_req, ospf_ls_req_timer, nbr);
}

/* Send Link State Request. */
void
ospf_ls_req_send (struct ospf_neighbor *nbr)
{
  struct ospf_interface *oi = nbr->oi;
  struct stream *s = oi->top->obuf;
  struct ospf_packet *op;

  /* Reset stream. */
  stream_reset (s);

  /* Build packet. */
  ospf_packet_header_set (s, oi, OSPF_PACKET_LS_REQ);   /* OSPF Header. */
  ospf_packet_ls_req_set (s, nbr);                      /* LS-Req body. */
  ospf_packet_header_complete (s, oi);                  /* Fill header. */

  /* Get the packet.  */
  op = ospf_packet_get (oi, OSPF_PACKET_LS_REQ, OSPF_PACKET_DST_BY_NBR (nbr));
  if (op == NULL)
    return;

  /* Add packet to the interface output queue. */
  ospf_packet_add (oi, op);
  oi->ls_req_out++;

  /* Hook thread to write packet. */
  OSPF_IFSM_WRITE_ON (oi);

  /* Add Link State Request Retransmission Timer. */
  OSPF_NFSM_TIMER_ON (nbr->t_ls_req, ospf_ls_req_timer, nbr->v_ls_req);
}


void
ospf_ls_upd_send_queue (struct ospf_interface *oi,
                        vector queue, struct pal_in4_addr *dst)
{
  struct ospf_master *om = oi->top->om;
  struct stream *s = oi->top->obuf;
  struct ospf_packet *op;
  int count;

  if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_LS_UPD, SEND))
    zlog_info (ZG, "SEND[%s]: %d LSAs to destination %r",
               LOOKUP (ospf_packet_type_msg, OSPF_PACKET_LS_UPD),
               vector_count (queue), dst);

  /* Reset stream. */
  stream_reset (s);

  /* Build packet. */
  ospf_packet_header_set (s, oi, OSPF_PACKET_LS_UPD);   /* OSPF Header. */
  count = ospf_packet_ls_upd_set (s, oi, queue);                /* LS-Upd body. */
 if (count == 0)
     {
       if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_LS_UPD, SEND))
       zlog_info (ZG, "SEND[%s]: no current LSAs to destination %r",
                  LOOKUP (ospf_packet_type_msg, OSPF_PACKET_LS_UPD),
                  dst);

       stream_reset (s);
       return;
     }


  ospf_packet_header_complete (s, oi);                  /* Fill header. */

  /* Get the packet.  */
  op = ospf_packet_get (oi, OSPF_PACKET_LS_UPD, dst);
  if (op == NULL)
    return;

  /* Add packet to the interface output queue. */
  ospf_packet_add (oi, op);
  oi->ls_upd_out++;

  /* Hook thread to write packet. */
  OSPF_IFSM_WRITE_ON (oi);
}

vector
ospf_ls_upd_queue_get (struct ospf_interface *oi, struct pal_in4_addr *dst)
{
  struct ls_prefix p;
  struct ls_node *rn;
  vector queue;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, *dst);
  rn = ls_node_get (oi->ls_upd_queue, &p);
  if ((queue = RN_INFO (rn, RNI_DEFAULT)) == NULL)
    {
      queue = vector_init (OSPF_VECTOR_SIZE_MIN);
      RN_INFO_SET (rn, RNI_DEFAULT, queue);
    }
  ls_unlock_node (rn);

  return queue;
}

void
ospf_ls_upd_queue_add (struct ospf_interface *oi,
                       struct ospf_lsa *lsa, struct pal_in4_addr *dst)
{
  vector queue = ospf_ls_upd_queue_get (oi, dst);

  if (vector_set (queue, ospf_lsa_lock (lsa)) >= OSPF_VECTOR_SIZE_MAX)
    ospf_ls_upd_send_queue (oi, queue, dst);
  else
    OSPF_EVENT_ON (oi->t_ls_upd_event, ospf_ls_upd_send_event, oi);
}

void
ospf_ls_upd_queue_clear (struct ospf_interface *oi, struct pal_in4_addr *dst)
{
  struct ospf_lsa *lsa;
  struct ls_prefix p;
  struct ls_node *rn;
  vector queue;
  int i;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, *dst);
  rn = ls_node_lookup (oi->ls_upd_queue, &p);
  
  if (rn)
    if ((queue = RN_INFO (rn, RNI_DEFAULT)) != NULL)
      {
        for (i = 0; i < vector_max (queue); i++)
          if ((lsa = vector_slot (queue, i)))
            ospf_lsa_unlock (lsa);

        vector_free (queue);
        RN_INFO_UNSET (rn, RNI_DEFAULT);
      }
}

void
ospf_ls_upd_send_lsa (struct ospf_interface *oi, struct ospf_lsa *lsa)
{
#ifdef HAVE_RESTART
  struct ospf_master *om = oi->top->om;
  struct ospf_neighbor *nbr;
  struct ls_node *rn;
  char nbr_str[OSPF_NBR_STRING_MAXLEN];

  if (!CHECK_FLAG (lsa->flags, OSPF_LSA_REFRESHED))
    if (IS_LSA_TYPE_CHECKED_FOR_RESTART (lsa))
      for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
        if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
          if (CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_RESTART))
            {
              if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
                zlog_info (ZG, "ROUTER: Exit Restart Helper mode for "
                           "neighbor(%s) by flooding LSA refresh",
                           NBR_NAME (nbr));

              ospf_restart_helper_exit (nbr);
            }
#endif /* HAVE_RESTART */

  ospf_ls_upd_queue_add (oi, lsa, OSPF_PACKET_DST_BY_IF (oi));
}

/* Send Link State Update with an LSA. */
void
ospf_ls_upd_send_lsa_direct (struct ospf_neighbor *nbr, struct ospf_lsa *lsa)
{
#ifdef HAVE_RESTART
  struct ospf_master *om = nbr->oi->top->om;
  char nbr_str[OSPF_NBR_STRING_MAXLEN];

  if (CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_RESTART))
    if (!CHECK_FLAG (lsa->flags, OSPF_LSA_REFRESHED))
      if (IS_LSA_TYPE_CHECKED_FOR_RESTART (lsa))
        {
          if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
            zlog_info (ZG, "ROUTER: Exit Restart Helper mode for neighbor(%s) "
                       "by flooding LSA refresh", NBR_NAME (nbr));

          ospf_restart_helper_exit (nbr);
        }
#endif /* HAVE_RESTART */

  ospf_ls_upd_queue_add (nbr->oi, lsa, OSPF_PACKET_DST_BY_NBR (nbr));
}

/* Force to send Link State Update with an LSA.  */
void
ospf_ls_upd_send_lsa_force (struct ospf_interface *oi,
                            struct ospf_lsa *lsa, struct pal_in4_addr *dst)
{
  vector queue;

  queue = vector_init (OSPF_VECTOR_SIZE_MIN);
  vector_set (queue, ospf_lsa_lock (lsa));

  ospf_ls_upd_send_queue (oi, queue, dst);

  vector_free (queue);
}

int
ospf_ls_upd_send_event (struct thread *thread)
{
  struct ospf_interface *oi = THREAD_ARG (thread);
  struct pal_in4_addr *dst;
  struct ls_node *rn;
  vector queue;
  struct ospf_master *om = oi->top->om;

  oi->t_ls_upd_event = NULL;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  for (rn = ls_table_top (oi->ls_upd_queue); rn; rn = ls_route_next (rn))
    if ((queue = RN_INFO (rn, RNI_DEFAULT)))
      {
        dst = (struct pal_in4_addr *)rn->p->prefix;

        while (vector_count (queue))
          ospf_ls_upd_send_queue (oi, queue, dst);

        vector_free (queue);
        RN_INFO_UNSET (rn, RNI_DEFAULT);
      }

  return 0;
}

int
ospf_ls_ack_count (struct thread *thread)
{
  struct ospf_neighbor *nbr = THREAD_ARG (thread);
  struct ospf_master *om = nbr->oi->top->om;

  nbr->t_ls_ack_cnt = NULL;

  /* Set VR context */
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  if (nbr->rxmt_count > OSPF_UPD_SENT_THRESHOLD) 
    {
      /* Checking if the neighbor is congested. If more than 75% of LSAs 
         are unacknowledged, neighbor is considered congested */
      if (nbr->rxmt_count > (HIGH_WATER_MARK_PERCENT * (nbr->rxmt_count + nbr->ack_count)))
        nbr->is_congested = 1;
      else if (nbr->rxmt_count < (LOW_WATER_MARK_PERCENT * (nbr->rxmt_count + nbr->ack_count)))
        nbr->is_congested = 0;
      else 
        nbr->is_congested = -1; /*percentage of unacknowledged LSAs between 25% and 75% */
    }
  return 0;
}                  		

int
ospf_ls_upd_timer_interval (struct ospf_neighbor *nbr)
{
  int interval = nbr->v_retransmit;
  int jitter = 0;
 
  /* If there is congestion, calculate new retransmit interval */
  if (nbr->is_congested > 0) 
    {    
      /* As per Recommendation 3 of RFC4222 */
      interval = K_RXMT_INTERVAL_FACTOR * interval; 
      if (interval > nbr->v_retransmit_max);         
        interval = nbr->v_retransmit_max;         
      nbr->v_retransmit = interval;
    } 
  else if (!nbr->is_congested) /*percentage of unacknowledged LSAs <25% */
    {
      interval = interval/K_RXMT_INTERVAL_FACTOR; 
      if (interval < OSPF_RETRANSMIT_INTERVAL_MIN);         
        interval = OSPF_RETRANSMIT_INTERVAL_MIN;         
      nbr->v_retransmit = interval;
    }
  /* If percentage of unacknowledged LSAs between 25% and 75%
     interval is not changed */
	
  jitter = interval * OSPF_RETRANSMIT_INTERVAL_JITTER;

  /* The next interval will be interval +/- jitter.  */
  interval += nbr->oi->top->jitter_seed++ % (jitter * 2 + 1) - jitter;
 	
  if (interval < OSPF_RETRANSMIT_INTERVAL_MIN)
    return (OSPF_RETRANSMIT_INTERVAL_MIN/ OSPF_RETRANSMIT_LISTS);
  else if (interval > OSPF_RETRANSMIT_INTERVAL_MAX)
    return (OSPF_RETRANSMIT_INTERVAL_MAX/ OSPF_RETRANSMIT_LISTS);
  else
    return (interval/ OSPF_RETRANSMIT_LISTS);
}

int
ospf_ls_upd_timer (struct thread *thread)
{
  struct ospf_neighbor *nbr = THREAD_ARG (thread);
  int interval = ospf_ls_upd_timer_interval (nbr);
  char nbr_str[OSPF_NBR_STRING_MAXLEN];
  struct ospf_master *om = nbr->oi->top->om;
  struct ospf_interface *oi = nbr->oi;
  struct ospf_lsdb *ls_rxmt;
  struct ospf_lsa *lsa;
  struct ls_node *rn;
  struct pal_in4_addr *dst;
  struct pal_timeval now;
  vector queue;
  int i;
#ifdef HAVE_SNMP
  u_int8_t count = 0;
#endif /* HAVE_SNMP */
  nbr->t_ls_upd = NULL;

  /* Set VR context */
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  if (IS_DEBUG_OSPF (nfsm, NFSM_TIMERS))
    zlog_info (ZG, "NFSM[%s]: LS update timer expire", NBR_NAME (nbr));

  /* Send Link State Update.  */
  if (ospf_ls_retransmit_count (nbr) > 0)
    {
      pal_time_tzcurrent (&now, NULL);

      /* Get vector for LS Update indexed by address. */
      dst = OSPF_PACKET_DST_BY_NBR (nbr);
      queue = ospf_ls_upd_queue_get (oi, dst);
      ls_rxmt = ospf_ls_retransmit_list_find (nbr);

      /* Set all retransmit LSA to vector. */
      LSDB_LOOP_ALL (ls_rxmt, i, rn, lsa)
        {
#ifdef HAVE_SNMP
          if (count++ < OSPF_MAX_RETX_TRAPS)
            {
              if (nbr->oi->type == OSPF_IFTYPE_VIRTUALLINK)
                ospfVirtIfTxRetransmit (oi->u.vlink, OSPF_PACKET_LS_UPD,
                                        (int)lsa->data->type, &lsa->data->id,
                                        &lsa->data->adv_router);
              else
                ospfTxRetransmit (nbr, oi->area, OSPF_PACKET_LS_UPD,
                                  (int)lsa->data->type, &lsa->data->id,
                                  &lsa->data->adv_router);
            }
#endif /* HAVE_SNMP */
          if (TV_CMP (TV_SUB (now, lsa->tv_update), INT2TV (interval)) >= 0)
            if (vector_set (queue, ospf_lsa_lock (lsa)) >= OSPF_VECTOR_SIZE_MAX)
              {
                ospf_ls_upd_send_queue (oi, queue, dst);
               
                if (nbr->t_ls_ack_cnt == NULL)
                  {
                    nbr->rxmt_count = 0;
                    /* Start timer for an interval of RXMT and check for ACK 
                       count*/
                    nbr->ack_count = 0;
                    OSPF_NFSM_TIMER_ON (nbr->t_ls_ack_cnt, ospf_ls_ack_count, 
                                        interval);
                  }
                nbr->rxmt_count += OSPF_VECTOR_SIZE_MAX;
              } 
        }

      if (vector_count (queue) > 0)
        {
          if (nbr->t_ls_ack_cnt != NULL)
            nbr->rxmt_count += vector_count (queue);

          OSPF_EVENT_ON (oi->t_ls_upd_event, ospf_ls_upd_send_event, oi);
        }
      else
        ospf_ls_upd_queue_clear (oi, dst);
    }

  /* Set LS Update retransmission timer again. */
  if (ospf_ls_retransmit_count (nbr) > 0)
    OSPF_NFSM_TIMER_ON (nbr->t_ls_upd, ospf_ls_upd_timer, interval);

  return 0;
}


int
ospf_ls_ack_direct_set (struct stream *s,
                        struct ospf_interface *oi, vector queue)
{
  struct lsa_header *lsah;
  int i;

  for (i = 0; i < vector_max (queue); i++)
    if ((lsah = vector_slot (queue, i)) != NULL)
      {
        if (stream_get_endp (s) + OSPF_LSA_HEADER_SIZE > ospf_packet_max (oi))
          return 1;

        ospf_buffer_stream_ensure (s, (stream_get_endp (s)
                                       + OSPF_LSA_HEADER_SIZE));

        stream_put (s, lsah, OSPF_LSA_HEADER_SIZE);
      }
  return 0;
}

int
ospf_ls_ack_send_direct (struct ospf_interface *oi, struct pal_in4_addr dst)
{
  struct stream *s = oi->top->obuf;
  struct ospf_packet *op;
  int ret;

  /* Reset stream. */
  stream_reset (s);

  /* Build packet. */
  ospf_packet_header_set (s, oi, OSPF_PACKET_LS_ACK);   /* OSPF Header. */
  ret = ospf_ls_ack_direct_set (s, oi, oi->direct.ls_ack); /* LS-Ack body. */
  ospf_packet_header_complete (s, oi);                  /* Fill header. */

  /* Get the packet.  */
  op = ospf_packet_get (oi, OSPF_PACKET_LS_ACK, &dst);
  if (op == NULL)
    return 0;

  /* Add packet to the interface output queue. */
  ospf_packet_add (oi, op);
  oi->ls_ack_out++;

  /* Hook thread to write packet. */
  OSPF_IFSM_WRITE_ON (oi);

  return ret;
}

int
ospf_ls_ack_send_queue (struct ospf_interface *oi,
                        vector queue, struct pal_in4_addr *dst)
{
  struct stream *s = oi->top->obuf;
  struct ospf_packet *op;
  int ret;

  /* Reset stream. */
  stream_reset (s);

  /* Build packet. */
  ospf_packet_header_set (s, oi, OSPF_PACKET_LS_ACK);   /* OSPF Header. */
  ret = ospf_packet_ls_ack_set (s, oi, queue);          /* LS-Ack body. */
  ospf_packet_header_complete (s, oi);                  /* Fill header. */

  /* Get the packet.  */
  op = ospf_packet_get (oi, OSPF_PACKET_LS_ACK, dst);
  if (op == NULL)
    return 0;

  /* Add packet to the interface output queue. */
  ospf_packet_add (oi, op);
  oi->ls_ack_out++;

  /* Hook thread to write packet. */
  OSPF_IFSM_WRITE_ON (oi);

  return ret;
}

int
ospf_ls_ack_send_queue_nbma (struct ospf_interface *oi)
{
  struct stream *s = oi->top->obuf;
  struct ospf_neighbor *nbr;
  struct ospf_packet *op;
  struct ls_node *rn;
  int count = 0;
  int ret;

  /* Reset stream. */
  stream_reset (s);

  /* Build packet. */
  ospf_packet_header_set (s, oi, OSPF_PACKET_LS_ACK);   /* OSPF Header. */
  ret = ospf_packet_ls_ack_set (s, oi, oi->ls_ack);     /* LS-Ack body. */
  ospf_packet_header_complete (s, oi);                  /* Fill header. */

  /* Send packet to each NBMA neighbor. */
  for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
    if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
      if (nbr->state >= NFSM_Exchange)
        {
          /* Get the packet.  */
          op = ospf_packet_get (oi, OSPF_PACKET_LS_ACK,
                                &nbr->ident.address->u.prefix4);
          if (op == NULL)
            return 0;

          /* Add packet to the interface output queue. */
          ospf_packet_add (oi, op);
          oi->ls_ack_out++;

          count++;
        }

  /* Hook thread to write packet. */
  if (count)
    OSPF_IFSM_WRITE_ON (oi);

  return ret;
}

/* Send Link State Acknowledgment delayed. */
void
ospf_ls_ack_send_delayed (struct ospf_interface *oi)
{
  /* RFC2328 Section 13.5 On non-broadcast networks, delayed
     Link State Acknowledgment packets must be unicast separately
     over each adjacency (i.e., neighbor whose state is >= Exchange).  */
  if (IS_OSPF_NETWORK_NBMA (oi))
    {
      while (ospf_ls_ack_send_queue_nbma (oi))
        ;
    }
  else
    while (ospf_ls_ack_send_queue (oi, oi->ls_ack, OSPF_PACKET_DST_BY_IF (oi)))
      ;
}

int
ospf_ls_ack_timer (struct thread *thread)
{
  struct ospf_interface *oi = THREAD_ARG (thread);
  struct ospf_master *om = oi->top->om;
  oi->t_ls_ack = NULL;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  /* Send Link State Acknowledgment. */
  if (vector_count (oi->ls_ack) > 0)
    {
      ospf_ls_ack_send_delayed (oi);
      OSPF_IFSM_TIMER_ON (oi->t_ls_ack, ospf_ls_ack_timer,
                          OSPF_DELAYED_ACK_DELAY);
    }

  return 0;
}


/* Check myself is in the neighbor list. */
int
ospf_hello_twoway_check (struct ospf_neighbor *nbr,
                         struct ospf_hello *hello, size_t size)
{
  struct ospf *top = nbr->oi->top;
  struct pal_in4_addr *nbr_id;
  u_char *p, *lim;

#ifdef HAVE_RESTART
  /* If neighbor's resync timer is running, skip 2-way check. */
  if (top->restart_method == OSPF_RESTART_METHOD_SIGNALING)
    if (nbr->t_resync != NULL)
      return 1;
#endif /* HAVE_RESTART */

  p = (u_char *)hello + OSPF_HELLO_MIN_SIZE;
  lim = (u_char *)hello + size;

  while (p < lim)
    {
      nbr_id = (struct pal_in4_addr *)p;
      if (IPV4_ADDR_SAME (&top->router_id, nbr_id))
        return 1;

      p += sizeof (struct pal_in4_addr);
    }

  return 0;
}

int
ospf_hello_is_nbr_changed (struct ospf_neighbor *nbr,
                           struct pal_in4_addr nbr_d_router,
                           struct pal_in4_addr nbr_bd_router,
                           u_char nbr_priority)
{
  struct pal_in4_addr nbr_addr = nbr->ident.address->u.prefix4;

  /* Not DR -> DR. */
  if (!IPV4_ADDR_SAME (&nbr_addr, &nbr_d_router)
      && IPV4_ADDR_SAME (&nbr_addr, &nbr->ident.d_router))
    return 1;

  /* DR -> Not DR. */
  if (IPV4_ADDR_SAME (&nbr_addr, &nbr_d_router)
      && !IPV4_ADDR_SAME (&nbr_addr, &nbr->ident.d_router))
    return 1;

  /* Not Backup -> Backup. */
  if (!IPV4_ADDR_SAME (&nbr_addr, &nbr_bd_router)
      && IPV4_ADDR_SAME (&nbr_addr, &nbr->ident.bd_router))
    return 1;

  /* Backup -> Not Backup. */
  if (IPV4_ADDR_SAME (&nbr_addr, &nbr_bd_router)
      && !IPV4_ADDR_SAME (&nbr_addr, &nbr->ident.bd_router))
    return 1;

  /* Priority changed. */
  if (nbr_priority != nbr->ident.priority)
    return 1;

  return 0;
}

/* OSPF Hello message read -- RFC2328 Section 10.5. */
int
ospf_hello (struct ospf_interface *oi, struct pal_in4_header *iph,
            struct ospf_header *ospfh, u_int16_t size)
{
  char if_str[OSPF_IF_STRING_MAXLEN];
  struct ospf_master *om = oi->top->om;
  struct stream *s = oi->top->ibuf;
  struct ospf_hello *hello;
  struct ospf_neighbor *nbr;
  struct ls_prefix p;
  struct pal_in4_addr nbr_d_router;
  struct pal_in4_addr nbr_bd_router;
  int nbr_priority;
  u_char prefixlen;
  u_char options;
#ifdef HAVE_RESTART
  u_int32_t eo_options = 0;
  u_int16_t auth_type;
#endif /* HAVE_RESTART */

  hello = (struct ospf_hello *)STREAM_PNT (s);

  /* If incoming interface is passive, ignore Hello. */
  if (CHECK_FLAG (oi->flags, OSPF_IF_PASSIVE))
    {
      if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_HELLO, RECV))
        zlog_warn (ZG, "RECV[Hello]: From %r via %s: Interface is passive",
                   &ospfh->router_id, IF_NAME (oi));
      return -1;
    }

  /* Get neighbor prefix. */
  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, iph->ip_src);
  prefixlen = ip_masklen (hello->network_mask);

  /* Compare network mask. */
  /* Checking is ignored for Point-to-Point and Virtual link. */
  if (oi->type != OSPF_IFTYPE_POINTOPOINT
      && oi->type != OSPF_IFTYPE_VIRTUALLINK)
    if (oi->ident.address->prefixlen != prefixlen)
      {
        if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_HELLO, RECV))
          zlog_warn (ZG, "RECV[Hello]: From %r via %s: NetworkMask mismatch",
                     &ospfh->router_id, IF_NAME (oi));
#ifdef HAVE_SNMP
        ospfIfConfigError (oi, iph->ip_src, OSPF_NET_MASK_MISMATCH,
                           ospfh->type);
#endif /* HAVE_SNMP */

        return -1;
      }

  /* Compare Hello Interval. */
  if (ospf_if_hello_interval_get (oi) != pal_ntoh16 (hello->hello_interval))
    {
      if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_HELLO, RECV))
        zlog_warn (ZG, "RECV[Hello]: From %r via %s: HelloInterval mismatch",
                   &ospfh->router_id, IF_NAME (oi));
#ifdef HAVE_SNMP
      if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
        ospfVirtIfConfigError (oi->u.vlink, OSPF_HELLO_INTERVAL_MISMATCH,
                               ospfh->type);
      else
        ospfIfConfigError (oi, iph->ip_src, OSPF_HELLO_INTERVAL_MISMATCH,
                           ospfh->type);
#endif /* HAVE_SNMP */

      return -1;
    }

  /* Compare Router Dead Interval. */
  if (ospf_if_dead_interval_get (oi) != pal_ntoh32 (hello->dead_interval))
    {
      if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_HELLO, RECV))
        zlog_warn (ZG, "RECV[Hello]: From %r via %s: "
                   "RouterDeadInterval mismatch", &ospfh->router_id,
                   IF_NAME (oi));
#ifdef HAVE_SNMP
      if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
        ospfVirtIfConfigError (oi->u.vlink, OSPF_DEAD_INTERVAL_MISMATCH,
                               ospfh->type);
      else
        ospfIfConfigError (oi, iph->ip_src, OSPF_DEAD_INTERVAL_MISMATCH,
                           ospfh->type);
#endif /* HAVE_SNMP */

      return -1;
    }

  /* Compare options. */
  options = ospf_hello_options (oi);
  if (CHECK_FLAG (options, HELLO_OPTIONS_MASK)
      != CHECK_FLAG (hello->options, HELLO_OPTIONS_MASK))
    {
      char option_str1[OSPF_OPTION_STR_MAXLEN];
      char option_str2[OSPF_OPTION_STR_MAXLEN];

      if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_HELLO, RECV))
        zlog_warn (ZG, "RECV[Hello]: From %r via %s: Options mismatch: "
                   "Local(%s) <-> Nbr(%s)", &ospfh->router_id, IF_NAME (oi),
                   ospf_option_dump (options, option_str1),
                   ospf_option_dump (hello->options, option_str2));
#ifdef HAVE_SNMP
      if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
        ospfVirtIfConfigError (oi->u.vlink, OSPF_OPTION_MISMATCH, ospfh->type);
      else
        ospfIfConfigError (oi, iph->ip_src, OSPF_OPTION_MISMATCH, ospfh->type);
#endif /* HAVE_SNMP */

      return -1;
    }

#ifdef HAVE_RESTART
  if (oi->top->restart_method == OSPF_RESTART_METHOD_SIGNALING)
    if (CHECK_FLAG (hello->options, OSPF_OPTION_L))
      {
        u_int16_t length = stream_get_endp (s) - pal_ntoh16 (ospfh->length);
        u_char *buf = STREAM_DATA (s) + pal_ntoh16 (ospfh->length);
        struct ospf_lls lls;
        
        auth_type = ospf_packet_auth_type (ospfh);
        if (auth_type == OSPF_AUTH_CRYPTOGRAPHIC)
          {
            length -= OSPF_AUTH_MD5_SIZE;
            buf += OSPF_AUTH_MD5_SIZE;
          }

        if (ospf_restart_lls_block_get (ospfh, oi, buf, length, &lls) < 0)
          return -1;

        /* Preserve Extenede Options. */
        eo_options = lls.eo.options;
      }
#endif /* HAVE_RESTART */

  /* Increment statistics.  */
  oi->hello_in++;

  /* Get neighbor structure. */
  nbr = ospf_nbr_get (oi, iph->ip_src, prefixlen);

  if (IS_OSPF_NETWORK_NBMA (oi))
    if (CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_STATIC))
      ospf_nbr_static_poll_timer_cancel (nbr);

  nbr->ident.router_id = ospfh->router_id;

#ifdef HAVE_RESTART
  if (oi->top->restart_method == OSPF_RESTART_METHOD_SIGNALING)
    ospf_restart_extended_options_check (nbr, eo_options);
#endif

  /* Add event to thread. */
  OSPF_NFSM_EVENT_EXECUTE (nbr, NFSM_HelloReceived);

  /*  RFC2328  Section 9.5.1
      If the router is not eligible to become Designated Router,
      (snip)   It must also send an Hello Packet in reply to an
      Hello Packet received from any eligible neighbor (other than
      the current Designated Router and Backup Designated Router).  */
  if (IS_OSPF_NETWORK_NBMA (oi))
    if ((oi->type == OSPF_IFTYPE_POINTOMULTIPOINT_NBMA
         && !CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_STATIC))
        || (oi->type == OSPF_IFTYPE_NBMA
            && oi->ident.priority == 0
            && hello->priority > 0
            && !IPV4_ADDR_SAME (&oi->ident.d_router, &iph->ip_src)
            && !IPV4_ADDR_SAME (&oi->ident.bd_router, &iph->ip_src)))
      OSPF_EVENT_EXECUTE (ospf_hello_reply_event, nbr, 0);

  /* Resume old parameter. */
  nbr_priority = nbr->ident.priority;
  nbr_d_router = nbr->ident.d_router;
  nbr_bd_router = nbr->ident.bd_router;

#ifdef HAVE_RESTART
  if (oi->top->restart_method != OSPF_RESTART_METHOD_SIGNALING
      || !CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_RESTART))
#endif /* HAVE_RESTART */
    {
      /* Set new parameter from Hello. */
      nbr->ident.priority = hello->priority;
      nbr->ident.d_router = hello->d_router;
      nbr->ident.bd_router = hello->bd_router;
    }

  if (!ospf_hello_twoway_check (nbr, hello, size))
    OSPF_NFSM_EVENT_EXECUTE (nbr, NFSM_OneWayReceived);
  else
    {
      OSPF_NFSM_EVENT_EXECUTE (nbr, NFSM_TwoWayReceived);
      nbr->options |= hello->options;

#ifdef HAVE_RESTART
      if (oi->state == IFSM_Waiting || oi->state == IFSM_DROther)
        if (IS_GRACEFUL_RESTART (oi->top) || IS_RESTART_SIGNALING (oi->top))
          ospf_restart_set_dr_by_hello (nbr, hello);
#endif /* HAVE_RESTART */

      /* RFC2328 9.2 Events causing interface state changes

         BackupSeen

         The router has detected the existence or non-existence of a
         Backup Designated Router for the network.  This is done in
         one of two ways.  First, an Hello Packet may be received
         from a neighbor claiming to be itself the Backup Designated
         Router.  Alternatively, an Hello Packet may be received from
         a neighbor claiming to be itself the Designated Router, and
         indicating that there is no Backup Designated Router.  In
         either case there must be bidirectional communication with
         the neighbor, i.e., the router must also appear in the
         neighbor's Hello Packet.  This event signals an end to the
         Waiting state.  */
      if (oi->state == IFSM_Waiting)
        {
          if ((IPV4_ADDR_SAME (&NBR_ADDR (nbr), &hello->d_router)
               && IPV4_ADDR_SAME (&hello->bd_router, &OSPFRouterIDUnspec))
              || IPV4_ADDR_SAME (&NBR_ADDR (nbr), &hello->bd_router))
            OSPF_IFSM_EVENT_SCHEDULE (oi, IFSM_BackupSeen);
        }

      /* Neighbor changed. */
      if (!CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_INIT))
        if (ospf_hello_is_nbr_changed (nbr, nbr_d_router,
                                       nbr_bd_router, nbr_priority))
          OSPF_IFSM_EVENT_SCHEDULE (oi, IFSM_NeighborChange);
    }

  return 0;
}

/* Process rest of DD packet */
static int
ospf_db_desc_proc (struct stream *s, struct ospf_interface *oi,
                   struct ospf_neighbor *nbr, struct ospf_db_desc *dd,
                   u_int16_t size, struct pal_in4_header *iph,
                   struct ospf_header *ospfh)
{
  struct ospf_master *om = oi->top->om;
  struct ospf_lsa *find;
  struct ospf_ls_request *lsr;
  struct lsa_header *lsah;

  for (size -= OSPF_DB_DESC_MIN_SIZE; size >= OSPF_LSA_HEADER_SIZE;
       size -= OSPF_LSA_HEADER_SIZE)
    {
      lsah = (struct lsa_header *) STREAM_PNT (s);
      stream_forward (s, OSPF_LSA_HEADER_SIZE);

      /* Unknown LS type. */
      if (!IS_LSA_TYPE_KNOWN (lsah->type))
        {
          if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_DB_DESC, RECV))
            zlog_warn (ZG, "RECV[DD]: Unknown LS type(%d)", lsah->type);
#ifdef HAVE_SNMP
          if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
            ospfVirtIfRxBadPacket (oi->u.vlink, ospfh->type);
          else
            ospfIfRxBadPacket (oi, iph->ip_src, ospfh->type);
#endif /* HAVE_SNMP */
          OSPF_NFSM_EVENT_SCHEDULE (nbr, NFSM_SeqNumberMismatch);
          return -1;
        }

      if (LSA_FLOOD_SCOPE (lsah->type) == LSA_FLOOD_SCOPE_AS
          && !IS_AREA_DEFAULT (oi->area))
        {
          if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_DB_DESC, RECV))
            zlog_warn (ZG, "RECV[DD]: AS-scoped-LSA from stub or NSSA, "
                       "ID(%r), AdvRouter(%r)", &lsah->id, &lsah->adv_router);
          OSPF_NFSM_EVENT_SCHEDULE (nbr, NFSM_SeqNumberMismatch);
          return -1;
        }

      /*RFC 5243 Database Summary Optimization */
      /* Lookup LSA in Database Summary List */
      if (CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_DB_SUMMARY_OPT))
        { 
          find = ospf_lsdb_lookup_by_id (nbr->db_sum, lsah->type, lsah->id, lsah->adv_router);
          if (find && ospf_lsa_header_more_recent (lsah, find) >= 0)
            {
              /* Received LSA is not recent. */
             if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_DB_DESC, RECV))
               zlog_info (ZG, "RECV[DD]: LSA received Type(%d), ID(%r), "
                              "AdvRouter(%r) is same or more recent, " 
                              "therefore deleted from DB Summary List",
                               lsah->type, &lsah->id, &lsah->adv_router);

              ospf_lsdb_delete (nbr->db_sum, find);
            }
        }
      /* Lookup received LSA, then add LS request list. */
      find = ospf_lsa_lookup (oi, lsah->type, lsah->id, lsah->adv_router);
      if (!find || ospf_lsa_header_more_recent (lsah, find) > 0)
        {
          /* Create LS-request object. */
          lsr = ospf_ls_request_new (lsah);
          ospf_ls_request_add (nbr, lsr);
          OSPF_NFSM_TIMER_ON (nbr->t_ls_req, ospf_ls_req_timer, nbr->v_ls_req);
        }
      else
        {
          /* Received LSA is not recent. */
          if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_DB_DESC, RECV))
            zlog_info (ZG, "RECV[DD]: LSA received Type(%d), ID(%r), "
                       "AdvRouter(%r) is not recent",
                       lsah->type, &lsah->id, &lsah->adv_router);
          continue;
        }
    }

  /* Master */
  if (IS_DD_FLAGS_SET (&nbr->dd, FLAG_MS))
    {
      nbr->dd.seqnum++;
      /* Entire DD packet sent. */
      if (!IS_DD_FLAGS_SET (dd, FLAG_M) && !IS_DD_FLAGS_SET (&nbr->dd, FLAG_M))
        OSPF_NFSM_EVENT_SCHEDULE (nbr, NFSM_ExchangeDone);
      else
        /* Send new DD packet. */
        ospf_db_desc_send (nbr);
    }
  /* Slave */
  else
    {
      nbr->dd.seqnum = dd->seqnum;

      /* When master's more flags is not set. */
      if (!IS_DD_FLAGS_SET (dd, FLAG_M) && ospf_db_summary_isempty (nbr))
        {
          UNSET_FLAG (dd->flags, OSPF_DD_FLAG_M);
          OSPF_NFSM_EVENT_SCHEDULE (nbr, NFSM_ExchangeDone);
        }

      /* Send DD pakcet in reply. */
      ospf_db_desc_send (nbr);
    }

  /* Save received neighbor values from DD. */
  nbr->dd.recv = *dd;

  return 0;
}

/* OSPF Database Description message read -- RFC2328 Section 10.6. */
int
ospf_db_desc (struct ospf_interface *oi, struct pal_in4_header *iph,
              struct ospf_header *ospfh, u_int16_t size)
{
  struct ospf_master *om = oi->top->om;
  struct stream *s = oi->top->ibuf;
  struct ospf_db_desc dd;
  struct ospf_neighbor *nbr;
#ifdef HAVE_OPAQUE_LSA
  char nbr_str[OSPF_NBR_STRING_MAXLEN];
#endif /* HAVE_OPAQUE_LSA */
  char if_str[OSPF_IF_STRING_MAXLEN];
  int event = NFSM_TwoWayReceived;
  int ret = 0;

  dd.mtu = stream_getw (s);
  dd.options = stream_getc (s);
  dd.flags = stream_getc (s);
  dd.seqnum = stream_getl (s);

  if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
    nbr = ospf_nbr_lookup_ptop (oi);
  else
    nbr = ospf_nbr_lookup_by_addr (oi->nbrs, &iph->ip_src);
  if (nbr == NULL)
    {
      if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_DB_DESC, RECV))
        zlog_warn (ZG, "RECV[DD]: From %r via %s: Unknown Neighbor",
                   &ospfh->router_id, IF_NAME (oi));
      return -1;
    }

  /* Check MTU. */
  if (!ospf_if_mtu_ignore_get (oi)
      && dd.mtu > ospf_if_mtu_get (oi))
    {
      if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_DB_DESC, RECV))
        zlog_warn (ZG, "RECV[DD]: From %r via %s: MTU size is too large (%d)",
                   &ospfh->router_id, IF_NAME (oi), dd.mtu);
#ifdef HAVE_SNMP
      if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
        ospfVirtIfRxBadPacket (oi->u.vlink, ospfh->type);
      else
        ospfIfRxBadPacket (oi, iph->ip_src, ospfh->type);
#endif /* HAVE_SNMP */
      return -1;
    }

#ifdef HAVE_RESTART
  if (oi->top->restart_method == OSPF_RESTART_METHOD_SIGNALING)
    if (CHECK_FLAG (dd.options, OSPF_OPTION_L))
      if (ospf_restart_dd_resync_proc (nbr, ospfh, &dd) < 0)
        return -1;
#endif /* HAVE_RESTART */

  /* Increment statistics.  */
  oi->db_desc_in++;

  /* Add event to thread - this is to increase scalability as per RFC4222 */
  OSPF_NFSM_EVENT_EXECUTE (nbr, NFSM_HelloReceived);

  /* Process DD packet by neighbor state. */
  switch (nbr->state)
    {
    case NFSM_Down:
    case NFSM_Attempt:
      if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_DB_DESC, RECV))
        zlog_warn (ZG, "RECV[DD]: From %r via %s: Neighbor state is %s, "
                   "packet discarded", &ospfh->router_id, IF_NAME (oi),
                   LOOKUP (ospf_nfsm_state_msg, nbr->state));
      break;
    case NFSM_TwoWay:
      event = NFSM_AdjOK;
    case NFSM_Init:
      SET_FLAG (nbr->flags, OSPF_NEIGHBOR_DD_INIT);
      OSPF_NFSM_EVENT_EXECUTE (nbr, event);
      /* If the new state is ExStart, the processing of the current
         packet should then continue in this new state by falling
         through to case ExStart below.  */
      if (nbr->state != NFSM_ExStart)
        {
          UNSET_FLAG (nbr->flags, OSPF_NEIGHBOR_DD_INIT);
          break;
        }
    case NFSM_ExStart:
      /* Slave. */
      if (IS_DD_FLAGS_SET_ALL (&dd)
          && size == OSPF_DB_DESC_MIN_SIZE
          && IPV4_ADDR_CMP (&nbr->ident.router_id, &oi->top->router_id) > 0)
        {
          if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_DB_DESC, RECV))
            zlog_info (ZG, "RECV[DD]: From %r via %s: "
                       "Negotiation done (Slave)",
                       &ospfh->router_id, IF_NAME (oi));

          nbr->dd.seqnum = dd.seqnum;
          UNSET_FLAG (nbr->dd.flags, OSPF_DD_FLAG_MS);  /* Reset MS-bit. */
          UNSET_FLAG (nbr->dd.flags, OSPF_DD_FLAG_I);   /* Reset I-bit. */
          nbr->options |= dd.options;
        }
      /* Master. */
      else if (!IS_DD_FLAGS_SET (&dd, FLAG_MS)
               && !IS_DD_FLAGS_SET (&dd, FLAG_I)
               && dd.seqnum == nbr->dd.seqnum
               && IPV4_ADDR_CMP (&nbr->ident.router_id,
                                 &oi->top->router_id) < 0)
        {
          if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_DB_DESC, RECV))
            zlog_info (ZG, "RECV[DD]: From %r via %s: "
                       "Negotiation done (Master)",
                       &ospfh->router_id, IF_NAME (oi));

          UNSET_FLAG (nbr->dd.flags, OSPF_DD_FLAG_I);
          nbr->options |= dd.options;
        }
      else
        {
          if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_DB_DESC, RECV))
            zlog_warn (ZG, "RECV[DD]: From %r via %s: Negotiation fails, "
                       "packet discarded", &ospfh->router_id, IF_NAME (oi));
          break;
        }

#ifdef HAVE_OPAQUE_LSA
      if (IS_DEBUG_OSPF (nfsm, NFSM_STATUS))
        zlog_info (ZG, "NFSM[%s]: Neighbor is %sOpaque-capable",NBR_NAME (nbr),
                   IS_NBR_OPAQUE_CAPABLE (nbr) ? "" : "not ");
#endif /* HAVE_OPAQUE_LSA */

      OSPF_NFSM_EVENT_EXECUTE (nbr, NFSM_NegotiationDone);

      /* Continue processing rest of packet. */
      ret = ospf_db_desc_proc (s, oi, nbr, &dd, size, iph, ospfh);
      break;
    case NFSM_Exchange:
      if (IS_DD_DUP (&dd, &nbr->dd.recv))
        {
          /* Master: discard duplicated DD packet. */
          if (IS_DD_FLAGS_SET (&nbr->dd, FLAG_MS))
            {
              if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_DB_DESC, RECV))
                zlog_warn (ZG, "RECV[DD]: From %r via %s:"
                           "packet duplicated (Master)",
                           &ospfh->router_id, IF_NAME (oi));
            }
          else
            /* Slave: cause to retransmit the last Database Description. */
            {
              if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_DB_DESC, RECV))
                zlog_warn (ZG, "RECV[DD]: From %r via %s: "
                           "packet duplicated (Slave)",
                           &ospfh->router_id, IF_NAME (oi));
              ospf_db_desc_resend (nbr, PAL_TRUE);
            }
          break;
        }

      /* Otherwise DD packet should be checked. */
      /* Check Master/Slave bit mismatch */
      if (IS_DD_FLAGS_SET (&dd, FLAG_MS)
          != IS_DD_FLAGS_SET (&nbr->dd.recv, FLAG_MS))
        {
          if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_DB_DESC, RECV))
            zlog_warn (ZG, "RECV[DD]: From %r via %s: MS-bit mismatch, "
                       "DD flags in packet(0x%x) <-> nbr(0x%x)",
                       &ospfh->router_id, IF_NAME (oi),
                       dd.flags, nbr->dd.flags);

          OSPF_NFSM_EVENT_SCHEDULE (nbr, NFSM_SeqNumberMismatch);
          break;
        }

      /* Check initialize bit is set. */
      if (IS_DD_FLAGS_SET (&dd, FLAG_I))
        {
          if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_DB_DESC, RECV))
            zlog_warn (ZG, "RECV[DD]: From %r via %s: I-bit is set",
                       &ospfh->router_id, IF_NAME (oi));
          OSPF_NFSM_EVENT_SCHEDULE (nbr, NFSM_SeqNumberMismatch);
          break;
        }

      /* Check DD Options. */
      if (dd.options != nbr->dd.recv.options)
        {
          if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_DB_DESC, RECV))
            zlog_warn (ZG, "RECV[DD]: From %r via %s: Options mismatch",
                       &ospfh->router_id, IF_NAME (oi));
          OSPF_NFSM_EVENT_SCHEDULE (nbr, NFSM_SeqNumberMismatch);
          break;
        }

      /* Check DD sequence number. */
      if ((IS_DD_FLAGS_SET (&nbr->dd, FLAG_MS)
           && dd.seqnum != nbr->dd.seqnum)
          || (!IS_DD_FLAGS_SET (&nbr->dd, FLAG_MS)
              && dd.seqnum != nbr->dd.seqnum + 1))
        {
          if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_DB_DESC, RECV))
            zlog_warn (ZG, "RECV[DD]: From %r via %s: "
                       "Sequence Number mismatch",
                       &ospfh->router_id, IF_NAME (oi));
          OSPF_NFSM_EVENT_SCHEDULE (nbr, NFSM_SeqNumberMismatch);
          break;
        }

      /* Continue processing rest of packet. */
      ret = ospf_db_desc_proc (s, oi, nbr, &dd, size, iph, ospfh);
      break;
    case NFSM_Loading:
    case NFSM_Full:
      /* Check DD Options. */
      if (dd.options != nbr->dd.recv.options)
        {
          if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_DB_DESC, RECV))
            zlog_warn (ZG, "RECV[DD]: From %r via %s: Options mismatch",
                       &ospfh->router_id, IF_NAME (oi));
          OSPF_NFSM_EVENT_SCHEDULE (nbr, NFSM_SeqNumberMismatch);
          break;
        }

      if (IS_DD_DUP (&dd, &nbr->dd.recv))
        {
          if (IS_DD_FLAGS_SET (&nbr->dd, FLAG_MS))
            {
              /* Master should discard duplicate DD packet. */
              if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_DB_DESC, RECV))
                zlog_warn (ZG, "RECV[DD]: From %r via %s: "
                           "Duplicated, packet discarded",
                           &ospfh->router_id, IF_NAME (oi));
              break;
            }
          else
            {
              struct pal_timeval t;
              t = TV_SUB (oi->top->tv_current, nbr->dd.sent.ts);
              if (nbr->dd.sent.packet != NULL
                  && TV_CMP (t, INT2TV (nbr->v_inactivity)) < 0)
                {
                  /* In states Loading and Full the slave must resend
                     its last Database Description packet in response to
                     duplicate Database Description packets received
                     from the master.  For this reason the slave must
                     wait RouterDeadInterval seconds before freeing the
                     last Database Description packet.  Reception of a
                     Database Description packet from the master after
                     this interval will generate a SeqNumberMismatch
                     neighbor event. RFC2328 Section 10.8 */
                  ospf_db_desc_resend (nbr, PAL_TRUE);
                  break;
                }
            }
        }

      OSPF_NFSM_EVENT_SCHEDULE (nbr, NFSM_SeqNumberMismatch);
      break;
    default:
      if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_DB_DESC, RECV))
        zlog_warn (ZG, "RECV[DD]: NFSM illegal state, internal error");
      break;
    }

  return ret;
}

/* OSPF Link State Request Read -- RFC2328 Section 10.7. */
int
ospf_ls_req (struct ospf_interface *oi, struct pal_in4_header *iph,
             struct ospf_header *ospfh, u_int16_t size)
{
  char if_str[OSPF_IF_STRING_MAXLEN];
  struct ospf_master *om = oi->top->om;
  struct stream *s = oi->top->ibuf;
  struct ospf_neighbor *nbr;
  struct ospf_lsa *find;
  u_int32_t ls_type;
  struct pal_in4_addr ls_id;
  struct pal_in4_addr adv_router;
  struct pal_in4_addr *dst;
  vector queue;

  if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
    nbr = ospf_nbr_lookup_ptop (oi);
  else
    nbr = ospf_nbr_lookup_by_addr (oi->nbrs, &iph->ip_src);
  if (nbr == NULL)
    {
      if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_LS_REQ, RECV))
        zlog_warn (ZG, "RECV[LS-Req]: From %r via %s Unknown Neighbor",
                   &ospfh->router_id, IF_NAME (oi));
      return -1;
    }

  /* Neighbor State should be Exchange or later. */
  if (nbr->state != NFSM_Exchange
      && nbr->state != NFSM_Loading
      && nbr->state != NFSM_Full)
    {
      if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_LS_REQ, RECV))
        zlog_warn (ZG, "RECV[LS-Req]: From %r via %s: Neighbor state is %s, "
                   "packet discarded", &ospfh->router_id, IF_NAME (oi),
                   LOOKUP (ospf_nfsm_state_msg, nbr->state));
      return -1;
    }

  if (IS_OSPF_NETWORK_NBMA (oi))
    dst = OSPF_PACKET_DST_BY_NBR (nbr);
  else
    dst = OSPF_PACKET_DST_BY_IF (oi);

  queue = ospf_ls_upd_queue_get (oi, dst);

  /* Send Link State Update for ALL requested LSAs. */
  while (size > 0)
    {
      /* Get one slice of Link State Request. */
      ls_type = stream_getl (s);
      ls_id.s_addr = stream_get_ipv4 (s);
      adv_router.s_addr = stream_get_ipv4 (s);

      /* Verify LSA type. */
      if (!IS_LSA_TYPE_KNOWN (ls_type))
        {
          OSPF_NFSM_EVENT_SCHEDULE (nbr, NFSM_BadLSReq);
          ospf_ls_upd_queue_clear (oi, dst);

          if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_LS_REQ, RECV))
            zlog_warn (ZG, "RECV[LS-Req]: Unknown LS type(%d)", ls_type);
#ifdef HAVE_SNMP
          if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
            ospfVirtIfRxBadPacket (oi->u.vlink, ospfh->type);
          else
            ospfIfRxBadPacket (oi, iph->ip_src, ospfh->type);
#endif /* HAVE_SNMP */

          return -1;
        }

      /* Search proper LSA in LSDB. */
      find = ospf_lsa_lookup (oi, ls_type, ls_id, adv_router);
      if (find == NULL)
        {
          OSPF_NFSM_EVENT_SCHEDULE (nbr, NFSM_BadLSReq);
          ospf_ls_upd_queue_clear (oi, dst);

          if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_LS_REQ, RECV))
            zlog_warn (ZG, "RECV[LS-Req]: No matched LSA, Type(%d), "
                       "ID(%r), AdvRouter(%r)", ls_type, &ls_id, &adv_router);
          return -1;
        }

      ospf_ls_upd_queue_add (oi, find, dst);

      size -= 12;
    }

  /* Send rest of Link State Update.  */
  if (vector_count (queue))
    OSPF_EVENT_ON (oi->t_ls_upd_event, ospf_ls_upd_send_event, oi);
  else
    ospf_ls_upd_queue_clear (oi, dst);

  /* Increment statistics.  */
  oi->ls_req_in++;

  /* Add event to thread - this is to increase scalability as per RFC4222 */
  OSPF_NFSM_EVENT_EXECUTE (nbr, NFSM_HelloReceived);

  return 0;
}

#define LSA_CONTINUE(L,S)                                                     \
    do {                                                                      \
      if (IS_DEBUG_OSPF (event, EVENT_LSA))                                   \
        zlog_info (ZG, "LSA[%s]: Discard LSA(0x%x) %s",                       \
                   LSA_NAME (L), (L), (S));                                   \
      return 0;                                                               \
    } while (0)

#define LS_ACK_SEND_DIRECT(I,L)                                               \
    vector_set ((I)->direct.ls_ack, (L)->data);                               \

/* Process one LSA in LS update packet.  */
int
ospf_ls_upd_proc (struct ospf_neighbor *nbr, struct ospf_lsa *lsa)
{
  struct ospf_interface *oi = nbr->oi;
  struct ospf *top = oi->top;
  struct ospf_master *om = top->om;
  struct ospf_lsa *ls_ret, *current;
  char lsa_str[OSPF_LSA_STRING_MAXLEN];
  int ret = 1;

  /* Reject AS-scoped-LSA from STUB or NSSA. */
  if (LSA_FLOOD_SCOPE (lsa->data->type) == LSA_FLOOD_SCOPE_AS)
    if (!IS_AREA_DEFAULT (nbr->oi->area))
      LSA_CONTINUE (lsa, "Received AS-scoped-LSA from Stub or NSSA");

#ifdef HAVE_NSSA
  if (lsa->data->type == OSPF_AS_NSSA_LSA)
    if (!IS_AREA_NSSA (nbr->oi->area))
      LSA_CONTINUE (lsa, "Received NSSA-LSA in non-NSSA area");
#endif /* HAVE_NSSA */
  
  /* Find the LSA in the current database. */
  current = ospf_lsa_lookup (oi, lsa->data->type,
                             lsa->data->id, lsa->data->adv_router);

  /* If the LSA's LS age is equal to MaxAge, and there is currently
     no instance of the LSA in the router's link state database,
     and none of router's neighbors are in states Exchange or Loading,
     then take the following actions. */
  if (IS_LSA_MAXAGE (lsa)
      && current == NULL
      && (ospf_nbr_count_by_state (oi, NFSM_Exchange) +
          ospf_nbr_count_by_state (oi, NFSM_Loading)) == 0)
    {
      /* Response Link State Acknowledgment. */
      LS_ACK_SEND_DIRECT (oi, lsa);
      LSA_CONTINUE (lsa, "Received LSA whose age is equal to MaxAge");
    }

  /* (5) Find the instance of this LSA that is currently contained
     in the router's link state database.  If there is no
     database copy, or the received LSA is more recent than
     the database copy the following steps must be performed. */
  if (current == NULL
      || (ret = ospf_lsa_more_recent (current, lsa)) < 0)
    {
      struct ospf_lsa *new_lsa;

      /* Allocate a new LSA.  */
      new_lsa = ospf_lsa_get_without_time (oi->top, lsa->data, lsa->alloced);
      new_lsa->tv_update = lsa->tv_update;
      new_lsa->lsdb = lsa->lsdb;
      new_lsa->status = notEXPLORED; 

      /* Actual flooding procedure. */
      if (ospf_flood (top, nbr, current, new_lsa) < 0)
        {
          ospf_lsa_discard (new_lsa);
          LSA_CONTINUE (lsa, "Flooding fail");
        }

      return 0;
    }

  /* (6) Else, If there is an instance of the LSA on the sending
     neighbor's Link state request list, an error has occurred in
     the Database Exchange process.  In this case, restart the
     Database Exchange process by generating the neighbor event
     BadLSReq for the sending neighbor and stop processing the
     Link State Update packet. */

  if (ospf_ls_request_lookup (nbr, lsa->data))
    {
      OSPF_NFSM_EVENT_SCHEDULE (nbr, NFSM_BadLSReq);

      if (IS_DEBUG_OSPF (event, EVENT_LSA))
        zlog_warn (ZG, "LSA[%s]: Instance exists on "
                   "Link state request list", LSA_NAME (lsa));

      return -1;
    }

  /* If the received LSA is the same instance as the database copy
     (i.e., neither one is more recent) the following two steps
     should be performed: */
  if (ret == 0)
    {
      /* If the LSA is listed in the Link state retransmission list
         for the receiving adjacency, the router itself is expecting
         an acknowledgment for this LSA.  The router should treat the
         received LSA as an acknowledgment by removing the LSA from
         the Link state retransmission list.  This is termed an
         "implied acknowledgment". */

      ls_ret = ospf_ls_retransmit_lookup (nbr, lsa);

      if (ls_ret != NULL)
        {
          ospf_ls_retransmit_delete (nbr, ls_ret);

          /* Delayed acknowledgment sent if advertisement received
             from Designated Router, otherwise do nothing. */
          if (oi->state == IFSM_Backup)
            if (IS_NBR_DECLARED_DR (nbr))
              {
                struct ospf_lsa *new_lsa;

                /* Allocate a new LSA.  */
                new_lsa = ospf_lsa_get_without_time
                  (oi->top, lsa->data, lsa->alloced);
                new_lsa->tv_update = lsa->tv_update;
                new_lsa->lsdb = lsa->lsdb;

                LS_ACK_ADD (oi, new_lsa);

                /* Discard the LSA after the delayed acknowledgment.  */
                ospf_lsa_discard (new_lsa);
              }

          LSA_CONTINUE (lsa, "Received LSA is the same instance "
                        "as the database copy, and on the "
                        "neighbor's retransmit list");
        }
      else
        /* Acknowledge the receipt of the LSA by sending a Link State
           Acknowledgment packet back out the receiving interface. */
        {
          LS_ACK_SEND_DIRECT (oi, lsa);
          LSA_CONTINUE (lsa, "Received LSA is the same instance "
                        "as the database copy");
        }
    }

  /* The database copy is more recent.  If the database copy
     has LS age equal to MaxAge and LS sequence number equal to
     MaxSequenceNumber, simply discard the received LSA without
     acknowledging it. (In this case, the LSA's LS sequence number is
     wrapping, and the MaxSequenceNumber LSA must be completely
     flushed before any new LSA instance can be introduced). */

  else if (ret > 0)  /* Database copy is more recent. */
    {
      if (IS_LSA_MAXAGE (current)
          && current->data->ls_seqnum == LSAMaxSeqNumber)
        {
          LSA_CONTINUE (lsa, "Received LSA is less recent, "
                        "and LS sequence number is wrapping");
        }
      /* Otherwise, as long as the database copy has not been sent in a
         Link State Update within the last MinLSArrival seconds, send the
         database copy back to the sending neighbor, encapsulated within
         a Link State Update Packet. The Link State Update Packet should
         be sent directly to the neighbor. In so doing, do not put the
         database copy of the LSA on the neighbor's link state
         retransmission list, and do not acknowledge the received (less
         recent) LSA instance. */
      else
        {
          /* LSA is considered to be different from current. */
          SET_FLAG (current->flags, OSPF_LSA_REFRESHED);

          if (TV_CMP (TV_SUB (oi->top->tv_current, current->tv_update),
                      INT2TV (OSPF_MIN_LS_ARRIVAL)) > 0)
            /* Trap NSSA type later. */
            ospf_ls_upd_send_lsa_direct (nbr, current);

          LSA_CONTINUE (lsa, "Received LSA is less recent");
        }
    }

  return 0;
}

/* Get the list of LSAs from Link State Update packet.
   And process some validation -- RFC2328 Section 13. (1)-(2). */
int
ospf_ls_upd_validate_proc (struct stream *s, struct ospf_interface *oi,
                           size_t size, struct ospf_neighbor *nbr,
                           struct pal_in4_header *iph,
                           struct ospf_header *ospfh)
{
  struct ospf_master *om = oi->top->om;
  struct lsa_header *lsah;
  struct ospf_lsa lsa;
  u_int16_t count, sum;
  u_int32_t length;
  char lsa_str[OSPF_LSA_STRING_MAXLEN];
  int ret;
  int packet_invalid = 1;

  count = stream_getl (s);
  size -= 4;

  /* Reset direct LS Ack vector.  */
  vector_reset (oi->direct.ls_ack);

  for (; size > 0 && count > 0;
       size -= length, stream_forward (s, length), count--)
    {
      lsah = (struct lsa_header *) STREAM_PNT (s);
      length = pal_ntoh16 (lsah->length);

      if (length > size)
        {
          if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_LS_UPD, RECV))
            zlog_warn (ZG, "RECV[LS-Upd]: LSA length exceeds packet size");
#ifdef HAVE_SNMP
          if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
            ospfVirtIfRxBadPacket (oi->u.vlink, ospfh->type);
          else
            ospfIfRxBadPacket (oi, iph->ip_src, ospfh->type);
#endif /* HAVE_SNMP */
          break;
        }

      /* Validate the LSA's LS checksum. */
      sum = lsah->checksum;
      if (sum != ospf_lsa_checksum (lsah))
        {
          if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_LS_UPD, RECV))
            zlog_warn (ZG, "RECV[LS-Upd]: LSA checksum error (%x) <-> (%x)",
                       sum, lsah->checksum);
          continue;
        }

      /* Examine the LSA's LS type. */
      if (!IS_LSA_TYPE_KNOWN (lsah->type))
        {
          if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_LS_UPD, RECV))
            zlog_warn (ZG, "RECV[LS-Upd]: Unknown LS type(%d)", lsah->type);
          continue;
        }

      /* Examine the LSA's LS age.  */
      if (pal_ntoh16 (lsah->ls_age) > OSPF_LSA_MAXAGE)
        {
          if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_LS_UPD, RECV))
            zlog_warn (ZG, "RECV[LS-Upd]: LS age is more than MaxAge(%hu)",
                       pal_ntoh16 (lsah->ls_age));
          continue;
        }

      /* Set LSA value to the temporary structure.  */
      lsa.data = lsah;
      lsa.alloced = length;
      lsa.tv_update = oi->top->tv_current;

      if ((lsa.lsdb = ospf_lsdb_get_by_type (oi, lsah->type)))
        {
          if (IS_DEBUG_OSPF (event, EVENT_LSA))
            zlog_info (ZG, "LSA[%s]: Instance(0x%x) created with "
                       "Link State Update", LSA_NAME (&lsa), &lsa);

          /* Process LSA.  */
          packet_invalid = 0;
          ret = ospf_ls_upd_proc (nbr, &lsa);
          if (ret < 0)
            return ret;
        }
    }

  /* If direct LS Ack is there, send it.  */
  if (vector_max (oi->direct.ls_ack))
    ospf_ls_ack_send_direct (oi, nbr->ident.address->u.prefix4);

  if(packet_invalid)
    return -1;
  else
    return 0;
}

/* OSPF Link State Update message read -- RFC2328 Section 13. */
int
ospf_ls_upd (struct ospf_interface *oi, struct pal_in4_header *iph,
             struct ospf_header *ospfh, u_int16_t size)
{
  char if_str[OSPF_IF_STRING_MAXLEN];
  struct ospf_master *om = oi->top->om;
  struct stream *s = oi->top->ibuf;
  struct ospf_neighbor *nbr;
  int ret;

  /* Check neighbor. */
  if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
    nbr = ospf_nbr_lookup_ptop (oi);
  else
    nbr = ospf_nbr_lookup_by_addr (oi->nbrs, &iph->ip_src);
  if (nbr == NULL)
    {
      if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_LS_UPD, RECV))
        zlog_warn (ZG, "RECV[LS-Upd]: From %r via %s: Unknown Neighbor",
                   &ospfh->router_id, IF_NAME (oi));
      return -1;
    }

  /* Check neighbor state. */
  if (nbr->state < NFSM_Exchange)
    {
      if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_LS_UPD, RECV))
        zlog_warn (ZG, "RECV[LS-Upd]: From %r via %s: Neighbor state is "
                   "less than Exchange", &ospfh->router_id, IF_NAME (oi));
      return -1;
    }

  /* Process LS Update. */
  ret = ospf_ls_upd_validate_proc (s, oi, size, nbr, iph, ospfh);

  /* Increment statistics.  */
  if (ret == 0)
    oi->ls_upd_in++;

  /* Add event to thread - this is to increase scalability as per RFC4222 */
  OSPF_NFSM_EVENT_EXECUTE (nbr, NFSM_HelloReceived);

  return ret;
}

/* OSPF Link State Acknowledgment message read -- RFC2328 Section 13.7. */
int
ospf_ls_ack (struct ospf_interface *oi, struct pal_in4_header *iph,
             struct ospf_header *ospfh, u_int16_t size)
{
  char if_str[OSPF_IF_STRING_MAXLEN];
  struct ospf *top = oi->top;
  struct stream *s ;
  struct ospf_neighbor *nbr;
  struct ospf_master *om = NULL;

  if (top == NULL)
    return -1;
  
  om = top->om;
  if (om == NULL)
    return -1;
 
  s = top->ibuf;

  if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
    nbr = ospf_nbr_lookup_ptop (oi);
  else
    nbr = ospf_nbr_lookup_by_addr (oi->nbrs, &iph->ip_src);
  if (nbr == NULL)
    {
      if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_LS_ACK, RECV))
        zlog_warn (ZG, "RECV[LS-Ack]: From %r via %s: Unknown Neighbor",
                   &ospfh->router_id, IF_NAME (oi));
      return -1;
    }

  if (nbr->state < NFSM_Exchange)
    {
      if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_LS_ACK, RECV))
        zlog_warn (ZG, "RECV[LS-Ack]: From %r via %s: "
                   "State is less than Exchange",
                   &ospfh->router_id, IF_NAME (oi));

      return -1;
    }

  if (s == NULL)
    return -1;

  while (size > 0)
    {
      struct ospf_lsa *lsr;
      struct lsa_header *lsah = (struct lsa_header *)STREAM_PNT (s);

#ifdef HAVE_RESTART
      struct ls_node *rn;
#endif  /* HAVE_RESTART */

      size -= OSPF_LSA_HEADER_SIZE;
      stream_forward (s, OSPF_LSA_HEADER_SIZE);

      if (IS_LSA_TYPE_KNOWN (lsah->type))
        {
          lsr = ospf_ls_retransmit_lookup_by_id (nbr, lsah->type, lsah->id,
                                                 lsah->adv_router);
          if (lsr != NULL)
            {
              /*Counter to trace number of ACKs received*/
              nbr->ack_count++;
              if (ospf_lsa_header_more_recent (lsah, lsr) == 0)
                ospf_ls_retransmit_delete (nbr, lsr);
            }

#ifdef HAVE_RESTART

          if (IS_GRACEFUL_RESTART (top) && IS_OPAQUE_CAPABLE (top)
              && ospf_restart_reason_lookup (top) == OSPF_RESTART_REASON_RESTART)
            {
              if (ospf_ls_retransmit_isempty (nbr))
                {
                  if (!CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_GRACE_ACK_RECD))
                    SET_FLAG (nbr->flags, OSPF_NEIGHBOR_GRACE_ACK_RECD);
                }
              else
                {
                  UNSET_FLAG (nbr->flags, OSPF_NEIGHBOR_GRACE_ACK_RECD);
                }

              for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
                if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
                  if (CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_GRACE_ACK_RECD))
                    oi->grace_ack_recv_count++;

              if (oi->grace_ack_recv_count == oi->full_nbr_count)
                {
                  if (!CHECK_FLAG(oi->flags, OSPF_IF_GRACE_LSA_ACK_RECD))
                    SET_FLAG (oi->flags, OSPF_IF_GRACE_LSA_ACK_RECD);
                  else
                    UNSET_FLAG (oi->flags, OSPF_IF_GRACE_LSA_ACK_RECD);
                }

              /* Check grace lsa acknowledgements. */
              ospf_grace_lsa_recv_check (top);
            }
#endif /* HAVE_RESTART */

        }
      else
        if (IS_DEBUG_OSPF_PACKET (OSPF_PACKET_LS_ACK, RECV))
          zlog_info (ZG, "RECV[LS-Ack]: From %r via %s: Unknown Type of LSA",
                     &ospfh->router_id, IF_NAME (oi));
    }

  /* Increment statistics.  */
  oi->ls_ack_in++;

  /* Add event to thread - this is to increase scalability as per RFC4222 */
  OSPF_NFSM_EVENT_EXECUTE (nbr, NFSM_HelloReceived);

  return 0;
}


struct ospf_interface *
ospf_packet_vlink_associate (struct ospf_interface *oi_recv,
                             struct pal_in4_header *iph,
                             struct ospf_header *ospfh)
{
  struct ospf *top = oi_recv->top;
  struct ospf_master *om = top->om;
  struct ospf_vlink *vlink;
  struct ospf_interface *oi = oi_recv;
  char vlink_str[OSPF_VLINK_STRING_MAXLEN];

  vlink = ospf_vlink_entry_lookup (top, oi->area->area_id, ospfh->router_id);
  if (vlink)
    {
      if (!CHECK_FLAG (vlink->flags, OSPF_VLINK_UP))
        {
          if (IS_DEBUG_OSPF (event, EVENT_VLINK))
            zlog_info (ZG, "VLINK[%s]: receive packet, but link is down",
                       VLINK_NAME (vlink));

          return NULL;
        }
      return vlink->oi;
    }

  return NULL;
}
#ifdef HAVE_OSPF_MULTI_AREA
struct ospf_interface *
ospf_packet_multi_area_link_associate (struct ospf_interface *oi_recv,
                                       struct prefix_ipv4 *p,
                                       struct ospf_header *ospfh)
{
  struct ospf *top = oi_recv->top;
  struct ospf_multi_area_link *mlink = NULL;
  mlink = ospf_multi_area_link_entry_lookup (top, ospfh->area_id,
                                        p->prefix);
  if (mlink)
    if (mlink->oi && CHECK_FLAG (mlink->oi->flags, OSPF_IF_UP))
      return mlink->oi;
  return NULL;
}
#endif /* HAVE_OSPF_MULTI_AREA */

/* Unbound socket will accept any Raw IP packets if proto is matched.
   To prevent it, compare src IP address and i/f address with masking
   i/f network mask. */
static int
ospf_packet_netmask_check (struct ospf_interface *oi,
                           struct pal_in4_addr ip_src)
{
  struct pal_in4_addr mask, me, him;
#ifdef HAVE_OSPF_MULTI_AREA
  /* RFC 5185 Section 2.3.1 */
  if ((oi->type == OSPF_IFTYPE_POINTOPOINT &&
       !CHECK_FLAG (oi->flags, OSPF_MULTI_AREA_IF))
      || oi->type == OSPF_IFTYPE_VIRTUALLINK)
#else
  if (oi->type == OSPF_IFTYPE_POINTOPOINT
      || oi->type == OSPF_IFTYPE_VIRTUALLINK)
#endif /* HAVE_OSPF_MULTI_AREA */
    return 1;

  masklen2ip (oi->ident.address->prefixlen, &mask);

  me.s_addr = oi->ident.address->u.prefix4.s_addr & mask.s_addr;
  him.s_addr = ip_src.s_addr & mask.s_addr;

 if (IPV4_ADDR_SAME (&me, &him))
   return 1;

 return 0;
}

static int
ospf_packet_auth_check (struct ospf_interface *oi,
                        struct ospf_header *ospfh,
                        u_int16_t ip_len)
{
  struct ospf_crypt_key *ck;
  struct listnode *node;
  struct ospf_if_params *oip = NULL;
  struct ospf_neighbor *nbr;
  int ret = 0;
  u_int16_t auth_type;

  auth_type = ospf_packet_auth_type (ospfh);
  switch (auth_type)
    {
    case OSPF_AUTH_NULL:
      return 1;
    case OSPF_AUTH_SIMPLE:
      if (pal_mem_cmp (ospf_if_authentication_key_get (oi),
                       ospfh->u.auth_data, OSPF_AUTH_SIMPLE_SIZE) == 0)
        return 1;
      break;
    case OSPF_AUTH_CRYPTOGRAPHIC:
      /* This is very basic, the digest processing is elsewhere. 
       If received key_id is as same as list tail, this neighbor exit
       rollover state, go through all neighbor, if all not in rollover
       state, set all key in list except tail to PASSIVE 
       else, if not as same as tail's key_id, if the passive is set,
       reset it. */
      if (OSPF_IF_PARAM_CHECK (oi->params, AUTH_CRYPT))
        oip = oi->params;
      else if (OSPF_IF_PARAM_CHECK (oi->params_default, AUTH_CRYPT))
        oip = oi->params_default;

      if (oip == NULL || oip->auth_crypt == NULL)
        {
          if (ospfh->u.crypt.key_id == 0)
            return 1;

          break;
        }

      if (ospfh->u.crypt.auth_data_len == OSPF_AUTH_MD5_SIZE
          && pal_ntoh16 (ospfh->length) + OSPF_AUTH_MD5_SIZE <= ip_len)
        for (node = LISTHEAD (oip->auth_crypt); node; NEXTNODE (node))
          {
            if ((ck = GETDATA (node)))
              if (ck->key_id == ospfh->u.crypt.key_id)
                {
                  if (CHECK_FLAG (ck->flags, OSPF_AUTH_MD5_KEY_PASSIVE))
                    UNSET_FLAG (ck->flags, OSPF_AUTH_MD5_KEY_PASSIVE);

                  OSPF_TIMER_OFF (ck->t_passive); 

                  if (node == oip->auth_crypt->tail)
                    {
                      nbr = ospf_nbr_lookup_by_router_id (oi->nbrs, 
                                                          &ospfh->router_id);
                      if (nbr && (CHECK_FLAG (nbr->flags, 
                                              OSPF_NEIGHBOR_MD5_KEY_ROLLOVER)))
                        {
                          UNSET_FLAG (nbr->flags,
                                      OSPF_NEIGHBOR_MD5_KEY_ROLLOVER);
                          ret = ospf_nbr_message_digest_rollover_state_check (oi);
                        }
                      if (ret == 0)
                        ospf_if_params_message_digest_passive_set_all (oip);
                    }
                  else 
                    OSPF_TIMER_ON (ck->t_passive, 
                                   ospf_if_message_digest_passive_timer, ck,
                                   OSPF_ROUTER_DEAD_INTERVAL_DEFAULT);
                  return 1;
                }
          }
      break;
    default:
      break;
    }

  return 0;
}

int
ospf_packet_checksum (struct ospf_interface *oi, struct ospf_header *ospfh)
{
  char if_str[OSPF_IF_STRING_MAXLEN];
  struct ospf_master *om = oi->top->om;
  u_int32_t ret;
  u_int16_t sum;

  /* Clear auth_data for checksum. */
  pal_mem_set (ospfh->u.auth_data, 0, OSPF_AUTH_SIMPLE_SIZE);

  /* Keep checksum and clear. */
  sum = ospfh->checksum;
  pal_mem_set (&ospfh->checksum, 0, sizeof (u_int16_t));

  /* Calculate checksum. */
  ret = in_checksum ((u_int16_t *)ospfh, pal_ntoh16 (ospfh->length));
  ospfh->checksum = sum;

  if (ret != sum)
    {
      if (IS_DEBUG_OSPF_PACKET (ospfh->type, RECV))
        zlog_warn (ZG, "RECV[%s]: From %r via %s: "
                   "OSPF checksum error 0x%X/0x%X",
                   LOOKUP (ospf_packet_type_msg, ospfh->type),
                   &ospfh->router_id, IF_NAME (oi), ret, sum);
      return 0;
    }

  return 1;
}

/* OSPF Header verification. */
static int
ospf_packet_header_check (struct ospf_interface *oi,
                          struct pal_in4_header *iph,
                          struct ospf_header *ospfh)
{
  struct ospf_master *om = oi->top->om;
  char if_str[OSPF_IF_STRING_MAXLEN];
  u_int16_t auth_type;

  /* If packet is by myself, silently discard. */
  if (IPV4_ADDR_SAME (&ospfh->router_id, &oi->top->router_id))
    return -1;

  /* Check OSPF Version. */
  if (ospfh->version != OSPF_VERSION)
    {
      if (IS_DEBUG_OSPF_PACKET (ospfh->type, RECV))
        zlog_warn (ZG, "RECV[%s]: From %r via %s: Version number mismatch",
                   LOOKUP (ospf_packet_type_msg, ospfh->type),
                   &ospfh->router_id, IF_NAME (oi));
#ifdef HAVE_SNMP
      if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
        ospfVirtIfConfigError (oi->u.vlink, OSPF_BAD_VERSION, ospfh->type);
      else
        ospfIfConfigError (oi, iph->ip_src, OSPF_BAD_VERSION, ospfh->type);
#endif /* HAVE_SNMP */

      return -1;
    }

  /* Check Area ID. */
  if (!IPV4_ADDR_SAME (&oi->area->area_id, &ospfh->area_id))
    {
      if (IS_DEBUG_OSPF_PACKET (ospfh->type, RECV))
        zlog_warn (ZG, "RECV[%s]: From %r via %s: Invalid Area ID %r",
                   LOOKUP (ospf_packet_type_msg, ospfh->type),
                   &ospfh->router_id, IF_NAME (oi), &ospfh->area_id);

#ifdef HAVE_SNMP
      if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
        ospfVirtIfConfigError (oi->u.vlink, OSPF_AREA_MISMATCH, ospfh->type);
      else
        ospfIfConfigError (oi, iph->ip_src, OSPF_AREA_MISMATCH, ospfh->type);
#endif /* HAVE_SNMP */

      return -1;
    }

  /* Check network mask. */
  if (!ospf_packet_netmask_check (oi, iph->ip_src))
    {
      if (IS_DEBUG_OSPF_PACKET (ospfh->type, RECV))
        zlog_warn (ZG, "RECV[%s]: From %r via %s: Network address mismatch",
                   LOOKUP (ospf_packet_type_msg, ospfh->type),
                   &ospfh->router_id, IF_NAME (oi));
      return -1;
    }
#ifdef HAVE_OSPF_MULTI_INST
  if (oi->network && ospfh->instance_id != oi->network->inst_id)
   {
     if (IS_DEBUG_OSPF_PACKET (ospfh->type, RECV))
       zlog_warn (ZG, "RECV[%s]: From %r via %s: Instance ID mismatch",
                   LOOKUP (ospf_packet_type_msg, ospfh->type),
                   &ospfh->router_id, IF_NAME (oi));
     return -1;
   }
#endif /* HAVE_OSPF_MULTI_INST */

  auth_type = ospf_packet_auth_type (ospfh);
  
  /* Check authentication. */
  if (ospf_auth_type (oi) != auth_type)
    {
      if (IS_DEBUG_OSPF_PACKET (ospfh->type, RECV))
        zlog_warn (ZG, "RECV[%s]: From %r via %s: "
                   "Authentication type mismatch",
                   LOOKUP (ospf_packet_type_msg, ospfh->type),
                   &ospfh->router_id, IF_NAME (oi));
#ifdef HAVE_SNMP
      if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
        ospfVirtIfConfigError (oi->u.vlink, OSPF_AUTH_TYPE_MISMATCH,
                               ospfh->type);
      else
        ospfIfConfigError (oi, iph->ip_src, OSPF_AUTH_TYPE_MISMATCH,
                           ospfh->type);
#endif /* HAVE_SNMP */
      return -1;
    }

  if (!ospf_packet_auth_check (oi, ospfh, pal_ntoh16 (iph->ip_len)))
    {
      if (IS_DEBUG_OSPF_PACKET (ospfh->type, RECV))
        zlog_warn (ZG, "RECV[%s]: From %r via %s: Authentication error",
                   LOOKUP (ospf_packet_type_msg, ospfh->type),
                   &ospfh->router_id, IF_NAME (oi));
#ifdef HAVE_SNMP
      if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
        ospfVirtIfConfigError (oi->u.vlink, OSPF_AUTH_FAILURE, ospfh->type);
      else
        ospfIfConfigError (oi, iph->ip_src, OSPF_AUTH_FAILURE, ospfh->type);
#endif /* HAVE_SNMP */

      return -1;
    }
  
  auth_type = ospf_packet_auth_type (ospfh);
  /* If check sum is invalid, packet is discarded. */
  if (auth_type != OSPF_AUTH_CRYPTOGRAPHIC)
    {
      if (!ospf_packet_checksum (oi, ospfh))
        {
#ifdef HAVE_SNMP
          if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
            ospfVirtIfRxBadPacket (oi->u.vlink, ospfh->type);
          else
            ospfIfRxBadPacket (oi, iph->ip_src, ospfh->type);
#endif /* HAVE_SNMP */

          return -1;
        }
    }
  else
    {
      if (ospfh->checksum != 0)
        {
#ifdef HAVE_SNMP
          if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
            ospfVirtIfRxBadPacket (oi->u.vlink, ospfh->type);
          else
            ospfIfRxBadPacket (oi, iph->ip_src, ospfh->type);
#endif /* HAVE_SNMP */

          return -1;
        }

#ifdef HAVE_MD5
      if (ospf_packet_md5_digest_check (oi, pal_ntoh16 (ospfh->length)) == 0)
        {
          if (IS_DEBUG_OSPF_PACKET (ospfh->type, RECV))
            zlog_warn (ZG, "RECV[%s]: From %r via %s: "
                       "MD5 authentication error",
                       LOOKUP (ospf_packet_type_msg, ospfh->type),
                       &ospfh->router_id, IF_NAME (oi));
#ifdef HAVE_SNMP
          if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
            ospfVirtIfConfigError (oi->u.vlink, OSPF_AUTH_FAILURE,
                                   ospfh->type);
          else
            ospfIfConfigError (oi, iph->ip_src, OSPF_AUTH_FAILURE,
                               ospfh->type);
#endif /* HAVE_SNMP */

          return -1;
        }
#endif /* HAVE_MD5 */
    }

  return 0;
}

int
ospf_packet_match_source (struct ospf_interface *oi,
                          struct interface *ifp, struct prefix_ipv4 *p)
{
  /* Special case for point-to-point.  */
  if (if_is_pointopoint (ifp))
    {
      if (if_is_ipv4_unnumbered (ifp))
        return 1;

      else if (oi->destination)
        if (IPV4_ADDR_SAME (&oi->destination->u.prefix4, &p->prefix))
          return 1;
    }

  if (prefix_match (oi->ident.address, (struct prefix *)p))
    return 1;

  return 0;
}

void
ospf_packet_proc_by_type (struct ospf_interface *oi,
                          struct pal_in4_header *iph,
                          struct ospf_header *ospfh)
{
  char if_str[OSPF_IF_STRING_MAXLEN];
  struct ospf_neighbor *nbr;
  u_int16_t length;
  u_int16_t auth_type;
  int ret = 0;

  /* Skip OSPF header and get length of OSPF packet body. */
  stream_forward (oi->top->ibuf, OSPF_HEADER_SIZE);
  length = pal_ntoh16 (ospfh->length) - OSPF_HEADER_SIZE;

  /* Read rest of the packet and call each sort of packet routine. */
  switch (ospfh->type)
    {
    case OSPF_PACKET_HELLO:
      ret = ospf_hello (oi, iph, ospfh, length);
      break;
    case OSPF_PACKET_DB_DESC:
      ret = ospf_db_desc (oi, iph, ospfh, length);
      break;
    case OSPF_PACKET_LS_REQ:
      ret = ospf_ls_req (oi, iph, ospfh, length);
      break;
    case OSPF_PACKET_LS_UPD:
      ret = ospf_ls_upd (oi, iph, ospfh, length);
      break;
    case OSPF_PACKET_LS_ACK:
      ret = ospf_ls_ack (oi, iph, ospfh, length);
      break;
    default:
      zlog_warn (ZG, "RECV[Type%d]: From %r via %s: Invalid OSPF packet type",
                 ospfh->type, &ospfh->router_id, IF_NAME (oi));
      ret = -1;
      break;
    }

  if (ret < 0)
    oi->discarded++;
 
  auth_type = ospf_packet_auth_type (ospfh);
 
  /* Save neighbor's crypt sequence number. */
  if (ospfh->auth_type == OSPF_AUTH_CRYPTOGRAPHIC)
    {
      nbr = ospf_nbr_lookup_by_router_id (oi->nbrs, &ospfh->router_id);
      if (nbr)
        nbr->crypt_seqnum = pal_ntoh32 (ospfh->u.crypt.crypt_seqnum);
    }
}

int
ospf_packet_proc_clone (struct ospf_interface *oi,
                        struct pal_in4_header *iph, struct ospf_header *ospfh,
                        unsigned long pp, struct prefix_ipv4 *q)
{
  char if_str[OSPF_IF_STRING_MAXLEN];
  struct stream *s = oi->top->ibuf;
  struct interface *ifp = oi->u.ifp;
  struct ospf *top = oi->top;
  struct ospf_master *om = top->om;
  struct ospf_interface *oi_dst = NULL;

  /* Set the get pointer.  */
  stream_set_getp (s, pp);

  /* Lookup virtual-link. */
  if (!IN_MULTICAST (pal_ntoh32 (iph->ip_dst.s_addr))
      && IS_AREA_BACKBONE (ospfh)
      && !IS_AREA_BACKBONE (oi->area))
    {
      /* Associate packet with Virtual-Link or Multi area link on back bone. */
      oi_dst = ospf_packet_vlink_associate (oi, iph, ospfh);
      if (oi_dst == NULL)
#ifdef HAVE_OSPF_MULTI_AREA
        oi_dst = ospf_packet_multi_area_link_associate (oi, q, ospfh);
      if (oi_dst == NULL) 
#endif /* HAVE_OSPF_MULTI_AREA */
        return 0;
      oi = oi_dst;
    }
  else
    {
#ifdef HAVE_OSPF_MULTI_AREA
      if (!IPV4_ADDR_SAME (&oi->area->area_id, &ospfh->area_id))
        {
          oi = ospf_packet_multi_area_link_associate (oi, q, ospfh);
          if (oi == NULL)
            return 0;
        }
      else
        {
#endif /* HAVE_OSPF_MULTI_AREA */
          /* IP source address does not match inteface. */
          if (!ospf_packet_match_source (oi, ifp, q))
            return 0;
#ifdef HAVE_OSPF_MULTI_AREA
        }
#endif /* HAVE_OSPF_MULTI_AREA */    
    }

  /* Check interface is still up. */
  if (!ospf_if_is_up (oi)
      || !CHECK_FLAG (oi->flags, OSPF_IF_UP)
      || CHECK_FLAG (oi->flags, OSPF_IF_DESTROY))
    return 0;

  /* Discard if IFSM state is not DR/Backup and
     destination address is AllDRouters. */
  if (IPV4_ADDR_SAME (&iph->ip_dst, &IPv4AllDRouters)
      && (oi->state != IFSM_DR && oi->state != IFSM_Backup))
    return 0;

  /* Show debug receiving packet. */
  if (IS_DEBUG_OSPF_PACKET (ospfh->type, RECV))
    {
      zlog_info (ZG, "RECV[%s]: From %r via %s (%r -> %r)",
                 LOOKUP (ospf_packet_type_msg, ospfh->type),
                 &ospfh->router_id, IF_NAME (oi), &iph->ip_src,
                 &iph->ip_dst);

      if (IS_DEBUG_OSPF_PACKET (ospfh->type, DETAIL))
        ospf_packet_dump (om, STREAM_DATA (top->ibuf));
    }

  /* Verify OSPF header. */
  if ((ospf_packet_header_check (oi, iph, ospfh)) < 0)
    return 0;

  /* Process packet by type. */
  ospf_packet_proc_by_type (oi, iph, ospfh);

  return 1;
}

int
ospf_packet_proc (struct ospf *top,
                  struct pal_in4_header *iph, struct interface *ifp)
{
  struct ospf_header *ospfh = (struct ospf_header *)STREAM_DATA (top->ibuf);
  struct ospf_master *om = top->om;
  struct ospf_interface *oi = NULL;
  struct ls_prefix8 p;
  struct ls_node *rn, *rn_tmp;
  struct prefix_ipv4 q;
  unsigned long pp;
#ifdef HAVE_OSPF_MULTI_INST
  struct ospf_interface *oi_alt = NULL;
#endif /* HAVE_OSPF_MULTI_INST */

  q.family = AF_INET;
  q.prefix = iph->ip_src;
  q.prefixlen = IPV4_MAX_BITLEN;

  p.prefixlen = 32;
  p.prefixsize = OSPF_GLOBAL_IF_TABLE_DEPTH;
  ls_prefix_set_args ((struct ls_prefix *)&p, "ia", &ifp->ifindex,
                      &IPv4AddressUnspec);

  /* Preserve pointer. */
  pp = stream_get_getp (top->ibuf);

  rn_tmp = ls_node_get (om->if_table, (struct ls_prefix *)&p);
  for (rn = ls_lock_node (rn_tmp); rn; rn = ls_route_next_until (rn, rn_tmp))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
      {
#ifdef HAVE_OSPF_MULTI_INST
        /* Get the entry from top->if_table */
        if (oi->top != top)
          {
            oi_alt = ospf_if_entry_match (top, oi);
            if (oi_alt)
              oi = oi_alt;
          }
#endif /* HAVE_OSPF_MULTI_INST */
        if (oi->u.ifp == ifp && oi->top == top)
          {
            if (oi->type == OSPF_IFTYPE_POINTOPOINT
                || oi->clone == 1)
              ospf_packet_proc_clone (oi, iph, ospfh, pp, &q);

            /* There is other interface which has the same network. */
            else
              {
                struct ospf_interface *alt;
                struct prefix p1, p2;
                struct listnode *node;

                prefix_copy (&p1, oi->ident.address);
                apply_mask (&p1);

                LIST_LOOP (oi->area->iflist, alt, node)
                  if (alt->type != OSPF_IFTYPE_POINTOPOINT && alt->clone > 1)
                    {
                      if (IN_MULTICAST (pal_ntoh32 (iph->ip_dst.s_addr)))
                        {
                          prefix_copy (&p2, alt->ident.address);
                          apply_mask (&p2);
                          if (prefix_same (&p1, &p2))
                            ospf_packet_proc_clone (alt, iph, ospfh, pp, &q);
                        }
                      else if (IPV4_ADDR_SAME (&iph->ip_dst,
                                             &alt->ident.address->u.prefix4))
                        ospf_packet_proc_clone (alt, iph, ospfh, pp, &q);
                    }
              }
          }
      }

  ls_unlock_node (rn_tmp);

  return 0;
}

struct interface *
ospf_packet_recv (struct ospf *top, struct pal_in4_header *iph)
{
  struct ospf_master *om = top->om;
  struct stream *s = top->ibuf;
  struct pal_iovec iov[2];
  struct interface *ifp;
  struct pal_msghdr msg;
  int ifindex;
  int ret;

  /* Allocate CMSG for PKTINFO/RECVIF.  */
  pal_sock_in4_cmsg_init (&msg, PAL_CMSG_IPV4_PKTINFO|PAL_CMSG_IPV4_RECVIF);

  /* Prepare the buffer.  */
  iov[0].iov_base = (void *)iph;
  iov[0].iov_len = IP_HEADER_SIZE;
  iov[1].iov_base = (void *)STREAM_DATA (s);
  iov[1].iov_len = stream_get_endp (s);

  /* Prepare the message header.  */
  msg.msg_iov = iov;
  msg.msg_iovlen = 2;

  /* Receive the packet.  */
  ret = pal_sock_recvmsg (top->fd, &msg, 0);
  if (ret < 0)
    {
      pal_sock_in4_cmsg_finish (&msg);
      zlog_warn (ZG, "RECV: recvmsg error: %s", pal_strerror (errno));
      return NULL;
    }

  /* Lookup the receving interface.  */
  ifindex = pal_sock_in4_cmsg_ifindex_get (&msg);
  if (ifindex != -1)
    ifp = if_lookup_by_index (&om->vr->ifm, ifindex);
  else
    ifp = if_match_by_ipv4_address (&om->vr->ifm, &iph->ip_src,
                                    top->ov->iv->id);

  /* Free CMSG.  */
  pal_sock_in4_cmsg_finish (&msg);

  return ifp;
}

u_int16_t
ospf_packet_length_get (struct pal_in4_header *iph)
{
  u_int32_t length = 0;

  /* Get the IP packet length.  */
  pal_in4_ip_packet_len_get (iph, &length);

  /* And subtract the IP header length.  */
  length -= iph->ip_hl << 2;

  if (length < OSPF_HEADER_SIZE)
    length = OSPF_HEADER_SIZE;

  return length;
}

/* Starting point of packet process function. */
int
ospf_read (struct thread *thread)
{
  struct ospf *top = THREAD_ARG (thread);
  pal_socklen_t len = sizeof (struct pal_sockaddr_in4);
  static struct pal_in4_header iph;
  struct pal_sockaddr_in4 sin;
  struct interface *ifp;
  u_int16_t length;
  int ret;
  struct ospf_master *om = top->om;

  /* Reset the thread. */
  top->t_read = NULL;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  /* Peek the packet to get the OSPF packet length from the IP header.  */
  ret = pal_sock_recvfrom (top->fd, (void *)&iph, IP_HEADER_SIZE, MSG_PEEK,
                           (struct pal_sockaddr *)&sin, &len);
  if (ret < 0)
    zlog_warn (ZG, "RECV: recvfrom error: %s", pal_strerror (errno));
  else
    {
      /* Get the OSPF packet length.  */
      length = ospf_packet_length_get (&iph);

      /* Reset the buffer.  */
      ospf_buffer_stream_ensure (top->ibuf, length);
      stream_reset (top->ibuf);
      stream_set_endp (top->ibuf, length);

      /* Read the OSPF packet.  */
      ifp = ospf_packet_recv (top, &iph);
      if (ifp != NULL)
        {
          /* Set the current time in top.  */
          pal_time_tzcurrent (&top->tv_current, NULL);

          /* Process the packet.  */
          ret = ospf_packet_proc (top, &iph, ifp);
        }
    }

  /* Prepare the read thread for the next packet.  */
  OSPF_READ_ON (top->t_read, ospf_read, top);

  return ret;
}
