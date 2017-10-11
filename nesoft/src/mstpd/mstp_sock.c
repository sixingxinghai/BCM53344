/* Copyright (C) 2003  All Rights Reserved. */

/* This module acts as the top half of the STP layer
   It will make pal calls to initialise, send & receive frames,
   and other STP related functions. */
#include "pal.h"

#ifdef HAVE_USER_HSL
#include "L2/hal_socket.h"
#endif /* HAVE_USER_HSL */

#include "lib.h"
#include "thread.h"
#include "l2lib.h"
#include "mstpd.h"
#include "mstp_config.h"
#include "mstp_types.h"
#include "mstp_bridge.h"
#include "l2_llc.h"
#include "l2_debug.h"
#include "mstpd.h"
#include "mstp_bpdu.h"
#include "mstp_sock.h"
#include "message.h"
#include "network.h"

#ifdef HAVE_HA
#include "mstp_cal.h"
#endif /* HAVE_HA */

#ifdef HAVE_SMI
#include "lib_fm_api.h"
#include "smi_message.h"
#include "smi_mstp_fm.h"
#endif /* HAVE_SMI */

#ifdef HAVE_I_BEB
#include "mstp_api.h"
#include "mstp_transmit.h"
#endif


#ifndef RCV_BUFSIZ
#define RCV_BUFSIZ  MSTP_PDU_SIZE
#endif /* RCV_BUFSIZ */

/* Local socket connection to forwarder */
static pal_sock_handle_t sockfd = -1;

#ifndef AF_STP
#define AF_STP            42
#endif
#ifndef AF_PROTO_MSTP
#define AF_PROTO_MSTP 3
#endif

#ifdef HAVE_USER_HSL
/*
   Asynchronous messages.
*/
struct mstp_client mstplink_async = { NULL };

#endif

static void
mstp_handle_frame (struct lib_globals *zg, struct sockaddr_vlan *l2_skaddr,
                   unsigned char *buf, int len);


#ifdef HAVE_USER_HSL

/* Initialize STP sub system.  */
pal_sock_handle_t
mstp_sock_init (struct lib_globals *zg)
{
  /* Open sockets to HSL. */
  sockfd = mstp_client_create (zg, &mstplink_async, MSTP_ASYNC);
  if (sockfd < 0)
    {
      if (LAYER2_DEBUG(proto, PROTO))
        zlog_debug (zg, "Can't initialize bridge feature.");

      return RESULT_ERROR;
    }

  return sockfd;
}


/* Send an STP frame through the selected port to the HSL server.
   This function returns the number of bytes transmitted or
   -1 if an error.
*/
int
mstp_send (const char * const brname,char * src_addr, const char *dest_addr, const u_int16_t vid,
           const u_int32_t ifindex , unsigned char *data, int length)
{
  int ret;
  u_int16_t tolen;
  struct sockaddr_vlan  l2_skaddr;
  u_int8_t send_buf [RCV_BUFSIZ];

  pal_mem_set (&l2_skaddr, 0, sizeof (struct sockaddr_vlan));

  if ( (sockfd < 0) || (data == NULL) )
   {

#ifdef HAVE_SMI

     smi_record_fault (mstpm, FM_GID_MSTP, MSTP_SMI_ALARM_TRANSPORT_FAILURE,
                       __LINE__, __FILE__,
     "STP failed to send data to HSL: no data or sockfd is invalid");

#endif /* HAVE_SMI */

     return RESULT_ERROR;
   }

#ifdef HAVE_HA
  if (HA_IS_STANDBY(mstpm))
    return RESULT_OK;
#endif /* HAVE_HA */
  pal_mem_set (&l2_skaddr, 0, sizeof(struct sockaddr_vlan));
  tolen = length + sizeof(struct sockaddr_vlan);

  /* Fill out the l2_skaddr structure here */
  l2_skaddr.port = ifindex;
  l2_skaddr.vlanid = vid;
  pal_mem_cpy (l2_skaddr.dest_mac, dest_addr, ETHER_ADDR_LEN);
  pal_mem_cpy (l2_skaddr.src_mac, src_addr, ETHER_ADDR_LEN);
  l2_skaddr.length = tolen;

  pal_mem_cpy (send_buf, &l2_skaddr, sizeof (struct sockaddr_vlan));
  pal_mem_cpy ((send_buf + sizeof (struct sockaddr_vlan)), data, length);

  ret = writen (mstplink_async.mc->sock, send_buf, tolen);

  if (ret != tolen)
  {

#ifdef HAVE_SMI
     smi_record_fault (mstpm, FM_GID_MSTP, MSTP_SMI_ALARM_TRANSPORT_FAILURE,
                       __LINE__, __FILE__,
     "STP failed to send data to HSL: writen failed");
#endif /* HAVE_SMI */

     return RESULT_ERROR;
  }

  return ret;
}

/* Async client Read from HSL server and parse frame. */
int
mstp_async_client_read (struct message_handler *mc, struct message_entry *me,
                        pal_sock_handle_t sock)
{
  int nbytes = 0;
  u_char buf[RCV_BUFSIZ];
  struct sockaddr_vlan vlan_skaddr;

  pal_mem_set (&vlan_skaddr, 0, sizeof (struct sockaddr_vlan));

  /* Peek at least Control Information */
  nbytes = pal_sock_recvfrom (sock, (void *)&vlan_skaddr,
                              sizeof (struct sockaddr_vlan),
                              MSG_PEEK, NULL, NULL);

  if (nbytes <= 0)
    {
      zlog_warn (mstpm, "PDU[RECV]: Peek failed (%d) length (%d)", nbytes,
                 vlan_skaddr.length);

#ifdef HAVE_SMI
      smi_record_fault (mstpm, FM_GID_MSTP, MSTP_SMI_ALARM_TRANSPORT_FAILURE,
                        __LINE__, __FILE__,
      "STP failed to receive data from HSL: pal_sock_recvfrom failed");
#endif /* HAVE_SMI */

      return nbytes;
    }

  nbytes = pal_sock_read (sock, buf, vlan_skaddr.length);

  if (nbytes <= 0)
    {
      zlog_warn (mstpm, "PDU[RECV]: receive failed (%d) length (%d)", nbytes,
                 vlan_skaddr.length);

#ifdef HAVE_SMI
      smi_record_fault(mstpm, FM_GID_MSTP, MSTP_SMI_ALARM_TRANSPORT_FAILURE,
                       __LINE__, __FILE__,
      "STP failed to receive data from HSL: pal_sock_read failed");
#endif /* HAVE_SMI */

      return nbytes;
    }

  if (nbytes < vlan_skaddr.length)
    zlog_warn (mstpm, "PDU[RECV]: Packet Truncated (%d)", nbytes);

#ifdef HAVE_HA
  if (HA_IS_STANDBY(mstpm))
    return RESULT_OK;
#endif /* HAVE_HA */

  pal_mem_cpy (&vlan_skaddr, buf, sizeof (struct sockaddr_vlan));

  /* Decode the raw packet and get src mac and dest mac */

  mstp_handle_frame (mstpm, &vlan_skaddr,( buf + sizeof(struct sockaddr_vlan)),
                    ( nbytes - sizeof(struct sockaddr_vlan)) );

  return nbytes;
}


/* Client connection is established.  Client send service description
   message to the server.  */
int
mstp_client_connect (struct message_handler *mc, struct message_entry *me,
                     pal_sock_handle_t sock)
{
  int ret;
  struct preg_msg preg;

  /* Make the client socket blocking. */
  pal_sock_set_nonblocking (sock, PAL_FALSE);

  /* Register read thread.  */
  if ((struct mstp_client *)mc->info == &mstplink_async)
    message_client_read_register (mc);

  preg.len  = MESSAGE_REGMSG_SIZE;
  preg.value = HAL_SOCK_PROTO_MSTP;

  /* Encode protocol identifier and send to HSL server */
  ret = pal_sock_write (sock, &preg, MESSAGE_REGMSG_SIZE);
  if (ret <= 0)
    {
      return RESULT_ERROR;
    }

  return RESULT_OK;

}

int
mstp_client_disconnect (struct message_handler *mc, struct message_entry *me,
                        pal_sock_handle_t sock)
{
  /* Stop message client.  */
  message_client_stop (mc);

  return RESULT_OK;
}

/* Make socket for netlink(RFC 3549) interface. */
int
mstp_client_create (struct lib_globals *zg, struct mstp_client *nl,
                    u_char async)
{
  struct message_handler *mc;
  int ret;

  /* Create async message client.  */
  mc = message_client_create (zg, MESSAGE_TYPE_ASYNC);
  if (mc == NULL)
    return -1;

#ifdef HAVE_TCP_MESSAGE
  message_client_set_style_tcp (mc, HSL_ASYNC_PORT);
#else
  message_client_set_style_domain (mc, HSL_ASYNC_PATH);
#endif /* HAVE_TCP_MESSAGE */

  /* Initiate connection using mstp client connection manager.  */
  message_client_set_callback (mc, MESSAGE_EVENT_CONNECT,
                               mstp_client_connect);
  message_client_set_callback (mc, MESSAGE_EVENT_DISCONNECT,
                               mstp_client_disconnect);
  if (async)
    message_client_set_callback (mc, MESSAGE_EVENT_READ_MESSAGE,
        mstp_async_client_read);

  /* Link each other.  */
  mc->info = nl;
  nl->mc = mc;

  /* Start the mstp client. */
  ret = message_client_start (nl->mc);

  return ret;
}

#else

/* Initialize STP sub system.  */
pal_sock_handle_t
mstp_sock_init (struct lib_globals *zg)
{
  sockfd = pal_sock(zg, AF_STP, SOCK_RAW, AF_PROTO_MSTP);
  if (sockfd < 0)
    {
      if (LAYER2_DEBUG(proto, PROTO))
        zlog_debug (zg, "Can't initialize bridge feature.");

      return RESULT_ERROR;
    }

  return sockfd;
}

/* Receive a frame via the pal. */
int
mstp_recv (struct thread *thread)
{
  struct sockaddr_vlan l2_skaddr;
  struct lib_globals *zg = thread->zg;
  u_char    data[RCV_BUFSIZ];
  int ret = 0;
  pal_socklen_t fromlen = sizeof(struct sockaddr_vlan);

  if (sockfd < 0)
    {
      zlog_err (zg, "PDU[RECV]: socket is not open (%d)", sockfd);
      return RESULT_ERROR;
    }

  /* Receive STP BPDU frame. */
  pal_mem_set (data, 0, RCV_BUFSIZ);

  ret = pal_sock_recvfrom(sockfd, (char *)data, RCV_BUFSIZ,MSG_TRUNC,
                          (struct pal_sockaddr *)&l2_skaddr, &fromlen);

  if (ret < 0)
    {
      zlog_warn (zg, "PDU[RECV]: receive failed (%d)", ret);

      /* Add read thread for next BPDU. */
      thread_add_read_high (zg, mstp_recv, NULL, sockfd);

      return ret;
    }

#ifdef HAVE_HA
  /* Drop bpdus on standby */
  if (HA_IS_STANDBY(zg))
    {
      /* Add read thread for next BPDU. */
      thread_add_read_high (zg, mstp_recv, NULL, sockfd);
      return RESULT_OK;
    }
#endif /* HAVE_HA */

  if (LAYER2_DEBUG (proto, PACKET_RX))
    {
      zlog_debug (zg, "PDU[RECV]: len (%d) "
                  "src addr (%.02x%.02x.%.02x%.02x.%.02x%.02x) "
                  "dst addr (%.02x%.02x.%.02x%.02x.%.02x%.02x) ",
                  ret,
                  l2_skaddr.src_mac[0],
                  l2_skaddr.src_mac[1],
                  l2_skaddr.src_mac[2],
                  l2_skaddr.src_mac[3],
                  l2_skaddr.src_mac[4],
                  l2_skaddr.src_mac[5],
                  l2_skaddr.dest_mac[0],
                  l2_skaddr.dest_mac[1],
                  l2_skaddr.dest_mac[2],
                  l2_skaddr.dest_mac[3],
                  l2_skaddr.dest_mac[4],
                  l2_skaddr.dest_mac[5]);

    }

   /* Handle BPDU. */
   mstp_handle_frame (zg, &l2_skaddr, data, ret);

  /* Add read thread for next BPDU. */
  thread_add_read_high (zg, mstp_recv, NULL, sockfd);

  return ret;
}

/* Send an STP frame through the selected port via the pal.
   This function returns the number of bytes transmitted or
   -1 if an error.
*/
int
mstp_send (const char * const brname,u_char * src_addr, const char *dest_addr,
           const u_int16_t vid, const u_int32_t ifindex , unsigned char *data,
           int length)
{
  int ret;
  int tolen;
  struct sockaddr_vlan  l2_skaddr;

  if ( (sockfd < 0) || (data == NULL) )
    {
      return RESULT_ERROR;
    }

#ifdef HAVE_HA
  if (HA_IS_STANDBY(mstpm))
    return RESULT_OK;
#endif /* HAVE_HA */

  tolen = sizeof(struct sockaddr_vlan);

  /* Fill out the l2_skaddr structure here */
  l2_skaddr.port = ifindex;
  l2_skaddr.vlanid = vid;
  pal_mem_cpy (l2_skaddr.dest_mac, dest_addr, ETHER_ADDR_LEN);
  pal_mem_cpy (l2_skaddr.src_mac, src_addr, ETHER_ADDR_LEN);

  ret = pal_sock_sendto (sockfd, (char *)data, length, 0,
                         (struct pal_sockaddr *)&l2_skaddr, tolen);
  if (ret < 0)
    return RESULT_ERROR;

  return ret;
}

#endif /* HAVE_USER_HSL */

static void
mstp_handle_frame (struct lib_globals *zg, struct sockaddr_vlan *l2_skaddr,
                   unsigned char *buf, int len)
{
  s_int32_t orig_len;
  u_int8_t *orig_buf;
  struct interface *ifp;
  struct LLC_Frame_s llc;
  struct mstp_bridge *br;
  struct mstp_port *port;
  struct mstp_bridge *ce_br;
  struct mstp_port *pe_port;
  struct mstp_port *ce_port;
  struct mstp_port *recv_port;
#ifdef HAVE_I_BEB
  struct mstp_port *cnp_port;
  u_char tunbuf[BR_MST_BPDU_MAX_FRAME_SIZE+VLAN_HEADER_SIZE];
  uint8_t pbb_dst_mac[ETHER_ADDR_LEN];
  uint32_t tunlen =0;
  struct mstp_port *pip_port;
  struct mstp_port *tmp_port = NULL;
#endif
#ifdef HAVE_B_BEB
  struct mstp_bridge *bcomp_br;
#endif
  struct apn_vr *vr = apn_vr_get_privileged ( mstpm );
#ifdef HAVE_RPVST_PLUS
  struct snap_frame snap;
  u_char sstp_buf[43];
  u_char *sstp_ptr;
#endif /* HAVE_RPVST_PLUS */

#ifdef HAVE_B_BEB
  bcomp_br = NULL;
#endif

  /* Validate the l2_skaddr data. Probably need to add length check. */
  if (l2_skaddr == NULL)
    return;

  ifp = if_lookup_by_index(&vr->ifm, l2_skaddr->port);

  if ( !ifp )
    return;

  br = mstp_find_bridge (ifp->bridge_name);

  if ((br == NULL) || (!br->mstp_enabled))
    return;

  recv_port = mstp_find_cist_port (br, ifp->ifindex);

  if ( !recv_port )
    return;

  port = NULL;
  ce_port = NULL;
  pe_port = NULL;
 
#ifdef HAVE_I_BEB 
  cnp_port = NULL;
  pal_mem_set (pbb_dst_mac, 0, ETHER_ADDR_LEN);
#endif

  orig_buf = buf;
  orig_len = len;

#ifdef HAVE_I_BEB
  if (MSTP_IS_PBB_BPDU (l2_skaddr->dest_mac) 
      && (recv_port->port_type == MSTP_VLAN_PORT_MODE_PIP 
           || recv_port->port_type == MSTP_VLAN_PORT_MODE_PNP))
    {
#if (!defined HAVE_B_BEB)
           ICOMP_MSTP_SEND_ON_CNP();
#else
           IBCOMP_MSTP_SEND_ON_CNP();
#endif /* HAVE_B_BEB */
         return;
    }
#endif /* HAVE_I_BEB */

  if (l2_llcParse (buf, &llc) == PAL_FALSE)
    {
       return;
    }

  buf += 3;
  len -= 3;

  if (LAYER2_DEBUG(proto, PROTO))
    {
      zlog_info (zg,
                 "mstp_handle_frame: received frame on port %d",
                 l2_skaddr->port);
            zlog_debug(mstpm, "mstp_handle_frame: BPDU Received");
    }
    recv_port->rx_count++;

#ifdef HAVE_RPVST_PLUS
  if (recv_port->br->type == MSTP_BRIDGE_TYPE_RPVST_PLUS)
    {
      if (MSTP_IS_RPVST_PLUS_BPDU(l2_skaddr->dest_mac))
        {
          if (l2_snap_parse(buf, &snap) == PAL_FALSE)
            {
              zlog_debug(mstpm, "mstp_handle_frame: SNAP Parsing failed");
              return;
            }

          /* Move ptr SNAP header length 5 bytes */
          buf += 5;
          len -= 5;

          if (len >= 42)
            {
              pal_mem_cpy(sstp_buf, buf, 42);
              sstp_ptr = sstp_buf;
              sstp_ptr += 40;

              recv_port->default_vlan = 0;
              recv_port->default_vlan = *sstp_ptr++ << 8; 
              recv_port->default_vlan = *sstp_ptr++; 

              if (recv_port->default_vlan == MSTP_VLAN_DEFAULT_VID)
                recv_port->rpvst_bpdu_type = RPVST_PLUS_BPDU_TYPE_CIST ; 
              else
                recv_port->rpvst_bpdu_type = RPVST_PLUS_BPDU_TYPE_SSTP ; 

              if (LAYER2_DEBUG(proto, PROTO))
                {  
                  zlog_info(zg, "mstp_handle_frame: received frame on vlan %d "
                      "len %d", recv_port->default_vlan, len);
                }
            }
          else
            recv_port->rpvst_bpdu_type = RPVST_PLUS_BPDU_TYPE_SSTP ;

        }
      else
        {
          recv_port->rpvst_bpdu_type = RPVST_PLUS_BPDU_TYPE_CIST ;
        }
    }
#endif

#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge (br);
#endif /* HAVE_HA */

#ifdef HAVE_RPVST_PLUS
  if (MSTP_IS_RPVST_PLUS_BPDU (l2_skaddr->dest_mac)
      && (recv_port->port_type == MSTP_VLAN_PORT_MODE_ACCESS
        || recv_port->port_type == MSTP_VLAN_PORT_MODE_TRUNK
        || recv_port->port_type == MSTP_VLAN_PORT_MODE_HYBRID))
    {
      mstp_handle_bpdu (recv_port, buf, len);
      return;
    }
#endif /* HAVE_RPVST_PLUS */

  if (MSTP_IS_PROVIDER_BPDU (l2_skaddr->dest_mac)
      && (recv_port->port_type == MSTP_VLAN_PORT_MODE_CN
          || recv_port->port_type == MSTP_VLAN_PORT_MODE_PN))
    {
      mstp_handle_bpdu (recv_port, buf, len);
      return;
    }

  if (MSTP_IS_CUSTOMER_BPDU (l2_skaddr->dest_mac)
      && (recv_port->port_type == MSTP_VLAN_PORT_MODE_ACCESS
          || recv_port->port_type == MSTP_VLAN_PORT_MODE_HYBRID
          || recv_port->port_type == MSTP_VLAN_PORT_MODE_TRUNK))
    {
      mstp_handle_bpdu (recv_port, buf, len);
      return;
    }

  if (MSTP_IS_CUSTOMER_BPDU (l2_skaddr->dest_mac)
      &&  recv_port->port_type == MSTP_VLAN_PORT_MODE_CE)
    {
      if ((ce_br = recv_port->ce_br))
        ce_port = mstp_find_ce_br_port (ce_br, MSTP_VLAN_NULL_VID);

      if (ce_port)
        mstp_handle_bpdu (ce_port, buf, len);

      return;
    }

  if (MSTP_IS_PROVIDER_BPDU (l2_skaddr->dest_mac)
      && recv_port->port_type == MSTP_VLAN_PORT_MODE_CE)
    {
      zlog_info (zg, "Discarding Provider BPDU On Customer Edge Port %d",
                 l2_skaddr->port);
      return;
    }

#ifdef HAVE_PROVIDER_BRIDGE
  for (port = br->port_list; port; port = port->next)
    {
      if (port->ce_br)
        {

          switch (port->ce_br->cust_bpdu_process)
            {
              case NSM_MSG_BRIDGE_PROTO_TUNNEL:
                mstp_send (port->br->name, l2_skaddr->src_mac, bridge_grp_add,
                           MSTP_VLAN_NULL_VID, port->ifindex,
                           orig_buf, orig_len);
                break;
              case NSM_MSG_BRIDGE_PROTO_DISCARD:
                break;
              case MSTP_BRIDGE_PROTO_PEER:
                pe_port = mstp_find_ce_br_port (port->ce_br,
                                                l2_skaddr->vlanid);

                if (pe_port)
                  mstp_handle_bpdu (pe_port, buf, len);

                break;
              default:
                break;
            }
        }
    }
#endif /* HAVE_PROVIDER_BRIDGE */

#if (defined HAVE_I_BEB)
  for (port = br->port_list; port; port = port->next)
     {
       if (port->cn_br)
         {
            switch(port->cn_br->cust_bpdu_process)
             {
               case NSM_MSG_BRIDGE_PROTO_TUNNEL:
                 for (cnp_port = port->cn_br->port_list;cnp_port;cnp_port=cnp_port->next)
                 {
                   if (cnp_port)
                    if (listcount(cnp_port->svid_list) > 0)
                      {
                      
                         pal_mem_set(tunbuf,0,sizeof(tunbuf));
                         tunlen = mstp_encapsulate_bpdu(tunbuf, cnp_port->isid,
                                                               l2_skaddr);

                         if (tunlen)
                           {
                              pal_mem_cpy(&tunbuf[tunlen],orig_buf, orig_len);
                              mstp_make_pbb_dst_mac(pbb_dst_mac,cnp_port->isid);
#ifdef HAVE_B_BEB
                              bcomp_br = (struct mstp_bridge *) \
                                                 mstp_find_bridge("backbone");
                              tmp_port = mstp_find_cist_port (bcomp_br, \
                                             cnp_port->pip_port);
#else
                              tmp_port = mstp_find_cist_port (port->br, \
                                             cnp_port->pip_port);
#endif /* HAVE_B_BEB */
                              if (tmp_port)
                                 mstp_send (port->br->name, port->dev_addr,
                                            pbb_dst_mac,tmp_port->bvid,
                                            tmp_port->ifindex,
                                            tunbuf, orig_len+tunlen);
                           }
                      }
                   }
             break;
               case NSM_MSG_BRIDGE_PROTO_DISCARD:
                 break;
               case MSTP_BRIDGE_PROTO_PEER:
                 /* TBD */
                 break;
               default:
                 break;
           }
        }
      }
#endif /* HAVE_I_BEB */
  return;
}

