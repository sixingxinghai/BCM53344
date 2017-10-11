/* Copyright 2003  All Rights Reserved.

  INET    Link Aggregation

  Authors:  McLendon
 */

#include "pal.h"
#include "cli.h"
#include "table.h"

#include "lacp_config.h"
#include "lacp_types.h"
#include "lacpd.h"
#include "lacp_timer.h"
#include "lacp_link.h"

#include "lacp_mux.h"
#include "lacp_churn.h"
#include "lacp_rcv.h"
#include "lacp_periodic_tx.h"
#include "lacp_selection_logic.h"
#include "lacp_tx.h"
#include "lacpdu.h"

#ifdef HAVE_HA
#include "cal_api.h"
#include "lacp_cal.h"
#endif /* HAVE_HA */

#ifdef HAVE_USER_HSL
#include "message.h"
#include "network.h"
#include "L2/hal_socket.h"
#endif /* HAVE_USER_HSL */

#ifdef HAVE_SMI
#include "lib_fm_api.h"
#include "smi_lacp_fm.h"
#endif /* HAVE_SMI */

#ifndef AF_LACP
#define AF_LACP            40
#endif /* AF_LACP */

#ifdef HAVE_USER_HSL
struct lib_globals *lacpm;

/* Local socket connection to forwarder */
static pal_sock_handle_t sockfd = -1;

/*
   Asynchronous messages.
*/
struct lacp_client lacplink_async = { NULL };

#endif /* HAVE_USER_HSL */

/* Management implementation */
int
lacp_set_system_priority (unsigned int priority)
{
  if (priority > 65535)
    return RESULT_ERROR;

  LACP_SYSTEM_PRIORITY = (unsigned short)priority;
  SET_FLAG (lacp_instance->config, LACP_CONFIG_SYSTEM_PRIORITY);
  lacp_instance->actor_change_count++;
    
#ifdef HAVE_HA
  lacp_cal_modify_instance ();
#endif /* HAVE_HA */

  return RESULT_OK;
}

/* Get system priority */
int
lacp_get_system_priority (unsigned int *priority)
{

  *priority = LACP_SYSTEM_PRIORITY;

  return RESULT_OK;
}

int
lacp_unset_system_priority ()
{
  LACP_SYSTEM_PRIORITY = LACP_DEFAULT_SYSTEM_PRIORITY;
  UNSET_FLAG (lacp_instance->config, LACP_CONFIG_SYSTEM_PRIORITY);
  lacp_instance->actor_change_count = 0;

#ifdef HAVE_HA
  lacp_cal_modify_instance ();
#endif /* HAVE_HA */

  return RESULT_OK;
}

int
lacp_set_channel_priority (struct lacp_link *link, unsigned int priority)
{
  int ret;

  if (! link || priority > 65536)
    return RESULT_ERROR;

  ret = lacp_set_link_priority (link, priority);

#ifdef HAVE_HA
  lacp_cal_modify_lacp_link (link);
#endif /* HAVE_HA */

  return ret;
}

/* Get channel priority */
int
lacp_get_channel_priority (struct lacp_link *link, unsigned int *priority)
{
  if (link == NULL)
    return RESULT_ERROR;

  *priority = link->actor_port_priority;

  return RESULT_OK;
}

int
lacp_unset_channel_priority (struct lacp_link *link)
{
  if (!link)
    return RESULT_ERROR;

  lacp_set_link_priority (link, LACP_DEFAULT_PORT_PRIORITY);

#ifdef HAVE_HA
  lacp_cal_modify_lacp_link (link);
#endif /* HAVE_HA */

  return RESULT_OK;
}

int
lacp_set_channel_timeout (struct lacp_link *link, int timeout)
{
  if (! link)
    return RESULT_ERROR;

  return lacp_set_link_timeout (link, timeout);
}

/* Get channel timeout */
int
lacp_get_channel_timeout (struct lacp_link *link, int *timeout)
{
  if (! link)
    return RESULT_ERROR;

  *timeout = GET_LACP_TIMEOUT (link->actor_oper_port_state);

  return RESULT_OK;
}

int
lacp_set_channel_admin_key (struct lacp_link *link, const unsigned int key)
{
  if (! link || key < 1 || key > 65535)
    return RESULT_ERROR;

  return lacp_set_link_admin_key (link, key);
}

/* Get channel admin key */
int
lacp_get_channel_admin_key (struct lacp_link *link, unsigned int *key)
{
  if (link == NULL)
    return RESULT_ERROR;

  *key = link->actor_admin_port_key;

  return RESULT_OK;
}

int
lacp_set_channel_activity (struct lacp_link *link, const unsigned int flag)
{
  if (link == NULL)
    return RESULT_ERROR;

  if (flag)
    {
      SET_LACP_ACTIVITY (link->actor_admin_port_state);
      SET_LACP_ACTIVITY (link->actor_oper_port_state);
    }
  else
    {
      CLR_LACP_ACTIVITY (link->actor_admin_port_state);
      CLR_LACP_ACTIVITY (link->actor_oper_port_state);
    }

#ifdef HAVE_HA
  lacp_cal_modify_lacp_link (link);
#endif /* HAVE_HA */

  return RESULT_OK;
}

#ifdef HAVE_SMI

/* Get channel activity */
int
lacp_get_channel_activity (struct lacp_link *link, enum smi_lacp_mode *mode)
{
  if (link == NULL)
    return RESULT_ERROR;

  *mode = GET_LACP_ACTIVITY(link->actor_oper_port_state);

  return RESULT_OK;
}

#endif /* HAVE_SMI */

int
lacp_aggregator_set_mac_address(struct lacp_aggregator *agg,
                          u_char *macaddr)
{
#if 0
  int ret = 0;
#endif

  if (! agg)
    return RESULT_ERROR;

#if 0
  if (FLAG_ISSET (agg->flag, LACP_AGG_FLAG_INSTALLED) &&
      agg->ifp)
    {
      ret = hal_if_set_hwaddr (agg->name, agg->ifp ? agg->ifp->ifindex : 0,
                               macaddr, 6);
    }

  return ret;
#endif

  return 0;
}



/* Run the state machines. These should be run every time:
   - a packet is received from a link
   - any timer expires */

void
lacp_run_sm (struct lacp_link *link)
{
  u_char sync_before;
  u_char sync_after;

#ifdef HAVE_HA
  lacp_cal_modify_lacp_link (link);
#endif /* HAVE_HA */

  if (IS_LACP_LINK_ENABLED (link))
    {
      sync_before = GET_SYNCHRONIZATION(link->actor_oper_port_state) & GET_SYNCHRONIZATION(link->partner_oper_port_state);
      lacp_rcv_sm (link);         /* Do this first */
      lacp_periodic_tx_sm (link);
      lacp_ready_logic ();
      lacp_mux_sm (link);
      lacp_churn_sm (link);
      lacp_tx (link);        /* Do this last */
      sync_after = GET_SYNCHRONIZATION(link->actor_oper_port_state) & GET_SYNCHRONIZATION(link->partner_oper_port_state);
      if (sync_before == 0 && sync_after == 1)
        {
          attach_mux_to_aggregator(link);
        } /* if sync_before == 0 and sync_after=1 */
      else if ((sync_before == 1) && (sync_after == 0))
        {
          detach_mux_from_aggregator(link, PAL_TRUE);
        }
    }
}


/* Handle an LACP PDU from the peer */
static struct lacp_pdu pdu;

int
lacp_handle_packet (int ifindex, u_char *data, const int len)
{
  struct lacp_link *link = lacp_find_link (ifindex);

  if (link == NULL || ! IS_LACP_LINK_ENABLED (link))
    return RESULT_ERROR;

  if (LACP_DEBUG(EVENT))
    zlog_info (LACPM, "lacp_handle_packet: link %s handle received"
               " frame of len %d", link->name, len);

  /* Here we peek into the packet to check if it's a marker PDU or a LACPDU. */
  if (data[0] == LACP_SUBTYPE)
    {
      /* Decode the packet and mark the appropriate bits in the link structure */
      if (parse_lacpdu (data, len, &pdu) != RESULT_OK)
        {
          link->pckt_recv_err_count++;
          return RESULT_ERROR;
        }

      if (LACP_DEBUG(PACKET))
        {
          zlog_debug (LACPM, "lacp_handle_packet: received lacp pdu"
                      " on link %d\n", link->actor_port_number);

          lacp_dump_packet (data);
        }

      /* Run the state machine for this link. */
      if (LACP_DEBUG(EVENT))
        zlog_debug (LACPM, "lacp_handle_packet: link %s run state machine",
                    link->name);

      link->lacpdu_recv_count++;
      link->pdu = &pdu;
      lacp_run_sm (link);
      link->pdu = NULL;
      link->last_pdu_rx_time = pal_time_current (NULL);
    }
  else if (data[0] == MARKER_SUBTYPE)
    {
      link->mpdu_recv_count++;
      if (parse_marker (data, len, &link->received_marker_info) != RESULT_OK)
        {
          link->pckt_recv_err_count++;
          return RESULT_ERROR;
        }
      if (link->received_marker_info.response == PAL_FALSE)
        {
          marker_tx (link);
        }
#ifdef HAVE_HA
      lacp_cal_modify_lacp_link (link);
#endif /* HAVE_HA */
    }

  return RESULT_OK;
}

/* CLI support routines below */

/* Spill our guts */
/* Display the traffic counts on a port*/
void
lacp_show_link_traffic_stats (struct cli *cli, const char *const ifname)
{
  struct lacp_link *link = lacp_find_link_by_name (ifname);
  if (link == NULL)
    {
      cli_out (cli, "%%Invalid interface: %s", ifname);
      return;
    }
  cli_out (cli, "%-9s%-8d%-8d%-8d%-8d%-8d%-8d\n",
           ifname, link->lacpdu_sent_count, link->lacpdu_recv_count,
           link->mpdu_sent_count, link->mpdu_recv_count,
           link->pckt_sent_err_count, link->pckt_recv_err_count);
}

static void
lacp_show_link_details (struct cli * cli, struct lacp_link * link)
{
  cli_out (cli, "%% LACP info for interface %s - %d",
      link->name, link->actor_port_number);
}

void
lacp_show_interface_stats (struct cli * cli, const char * const ifname)
{
  struct lacp_link * link = lacp_find_link_by_name (ifname);
  if (link == 0)
    {
      cli_out (cli, "%% Invalid interface: %s", ifname);
      return;
    }

  cli_out (cli, "%% LACP statistics for interface %s - %d",
      link->name, link->actor_port_number);
}

void
lacp_show_system (struct cli * cli)
{
}


void
lacp_show_all (struct cli * cli)
{
#if 0
  int hash;
  struct lacp_link * link;

  lacp_show_system (cli);
  for (hash = 0; hash < LACP_PORT_HASH_SIZE; hash++)
    {
      for (link = lacp_link_hash[hash]; link; link = link->next)
        {
          lacp_show_link_details (cli, link);
        }
    }
#endif
}

void
lacp_show_interface (struct cli * cli, const char * const ifname)
{
  struct lacp_link * link = lacp_find_link_by_name (ifname);
  lacp_show_system (cli);
  if (link)
    lacp_show_link_details (cli, link);
  else
    cli_out (cli, "%% Unable to find link %s", ifname);
}


int
lacp_config_write (struct cli * cli)
{
  if (FLAG_ISSET (lacp_instance->config, LACP_CONFIG_SYSTEM_PRIORITY))
    {
      cli_out (cli, "lacp system-priority %d\n", LACP_SYSTEM_PRIORITY);
      cli_out (cli, "!\n");
    }

  return RESULT_OK;
}


void
close_lacp ()
{
  pal_sock_close (LACPM, lacp_instance->fwd_socket);
  lacp_instance->fwd_socket = -1;
}


/* Open the LACP connection to the PAL */

int
open_lacp ()
{
  pal_sock_handle_t sock = -1;

  if (lacp_instance->fwd_socket >= 0)
    close_lacp ();

  sock = pal_sock (LACPM, AF_LACP, SOCK_RAW, 0);

  if (sock < 0)
    {
      zlog_err (LACPM, "open_lacp: Error opening socket (%d)", sock);
      return RESULT_ERROR;
    }

  lacp_instance->fwd_socket = sock;

  return RESULT_OK;
}



/* Peer callback - Handle an LACP frame */
int
recv_lacp (struct thread *thread)
{
  const char *fn_name = "recv_lacp";
  char    lacp_data[LACPDU_FRAME_SIZE];
  struct sockaddr_l2 peer_addr;
  pal_socklen_t peer_addr_len;
  int     ret = 0;

  lacp_instance->t_read = NULL;

  if (lacp_instance->fwd_socket < 0)
    {
      zlog_err (LACPM, "%s: lacp socket is not open", fn_name);
      return -1;
    }

  /* Receive LACP frame. */
  peer_addr_len = sizeof (struct sockaddr_l2);
  ret = pal_sock_recvfrom (lacp_instance->fwd_socket, lacp_data,
                           LACPDU_FRAME_SIZE, 0,
                           (struct pal_sockaddr *)&peer_addr,
                           &peer_addr_len);
  if (ret < 0)
    {
      zlog_warn (LACPM, "%s: lacp receive failed (%d)", fn_name, ret);
      return ret;
    }

#ifdef HAVE_HA
  if (HA_IS_STANDBY(LACPM))
    {
      if (! lacp_instance->t_read)
        lacp_instance->t_read = thread_add_read_high (LACPM, recv_lacp, NULL,
                                                      lacp_instance->fwd_socket);
      return RESULT_OK;
    }
#endif /* HAVE_HA */

  if (LACP_DEBUG (PACKET))
    {
      zlog_info (LACPM, "%s: len (%d) port(%d)\n"
          "  %s: src addr (%.02x%.02x.%.02x%.02x.%.02x%.02x) \n"
          "  %s: dst addr (%.02x%.02x.%.02x%.02x.%.02x%.02x)",
                 fn_name, ret, peer_addr.port,
                fn_name,
                 peer_addr.src_mac[0],
                 peer_addr.src_mac[1],
                 peer_addr.src_mac[2],
                 peer_addr.src_mac[3],
                 peer_addr.src_mac[4],
                 peer_addr.src_mac[5],
                 fn_name,
                 peer_addr.dest_mac[0],
                 peer_addr.dest_mac[1],
                 peer_addr.dest_mac[2],
                 peer_addr.dest_mac[3],
                 peer_addr.dest_mac[4],
                 peer_addr.dest_mac[5]);
    }


  /* Do the minimal PDU validation here */
  if (pal_mem_cmp (peer_addr.dest_mac, lacp_grp_addr, LACP_GRP_ADDR_LEN) == 0)
    {
       lacp_handle_packet (peer_addr.port, (u_char *) lacp_data, ret);
    }

  /* Immediately add read thread. */
  if (! lacp_instance->t_read)
    lacp_instance->t_read = thread_add_read_high (LACPM, recv_lacp, NULL,
                                                  lacp_instance->fwd_socket);

  return ret;
}

#ifdef HAVE_USER_HSL

/* Initialize LACP sub system.  */
pal_sock_handle_t
lacp_sock_init (struct lib_globals *zg)
{
  /* Open sockets to HSL. */
  sockfd = lacp_client_create (zg, &lacplink_async, LACP_ASYNC);
  if (sockfd < 0)
    {
        zlog_debug (zg, "Can't initialize bridge feature.");

      return RESULT_ERROR;
    }

  return sockfd;
}

/* Send an LACP frame through the selected port to the HSL server.
   This function returns the number of bytes transmitted or
   -1 if an error.
*/
int
send_lacp (struct sockaddr_l2 *addr, unsigned char *data, int len)
{
  int ret;
  u_int16_t tolen;
  u_int8_t send_buf [RCV_BUFSIZ];
  struct sockaddr_vlan vlan_addr;

  pal_mem_set (&vlan_addr, 0, sizeof (struct sockaddr_vlan));

  pal_mem_cpy (vlan_addr.dest_mac, addr->dest_mac, ETHER_ADDR_LEN);
  pal_mem_cpy (vlan_addr.src_mac, addr->src_mac, ETHER_ADDR_LEN);

  vlan_addr.port = addr->port;

  if ( (sockfd < 0) || (data == NULL) )
   {
#ifdef HAVE_SMI
     smi_record_fault(LACPM, FM_GID_LACP, LACP_SMI_ALARM_TRANSPORT_FAILURE,
                      __LINE__, __FILE__,
     "LACPD failed to send data to HSL: no data or sockfd is invalid");

#endif /* HAVE_SMI */

      return RESULT_ERROR;
   }

  tolen = len + sizeof(struct sockaddr_vlan);

  vlan_addr.length = tolen;

  pal_mem_cpy (send_buf, &vlan_addr, sizeof (struct sockaddr_vlan));
  pal_mem_cpy ((send_buf + sizeof (struct sockaddr_vlan)), data, len);

  ret = writen (lacplink_async.mc->sock, send_buf, tolen);

  if (ret != tolen)
  {
#ifdef HAVE_SMI
    smi_record_fault(LACPM, FM_GID_LACP, LACP_SMI_ALARM_TRANSPORT_FAILURE,
                     __LINE__, __FILE__,
    "LACPD failed to send data to HSL: writen failed");
#endif /* HAVE_SMI */
    return RESULT_ERROR;
  }

  return ret;
}

/* Async client Read from HSL server and parse frame. */
int
lacp_async_client_read (struct message_handler *mc, struct message_entry *me,
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
      zlog_warn (LACPM, "PDU[RECV]: receive failed (%d)", nbytes);
#ifdef HAVE_SMI
      smi_record_fault(LACPM, FM_GID_LACP, LACP_SMI_ALARM_TRANSPORT_FAILURE,
                       __LINE__, __FILE__,
      "LACPD failed to receive data from HSL: pal_sock_recvfrom failed");
#endif /* HAVE_SMI */
      return nbytes;
    }

  nbytes = pal_sock_read (sock, buf, vlan_skaddr.length);

  if (nbytes <= 0)
    {
      zlog_warn (LACPM, "PDU[RECV]: receive failed (%d)", nbytes);
#ifdef HAVE_SMI
      smi_record_fault(LACPM, FM_GID_LACP, LACP_SMI_ALARM_TRANSPORT_FAILURE,
                       __LINE__, __FILE__,
      "LACPD failed to receive data from HSL: pal_sock_read failed");
#endif /* HAVE_SMI */
      return nbytes;
    }

  if (nbytes < vlan_skaddr.length)
    zlog_warn (LACPM, "PDU[RECV]: Packet Truncated (%d)", nbytes);

  /* Decode the raw packet and get src mac and dest mac */
  pal_mem_cpy (&vlan_skaddr, buf, sizeof (struct sockaddr_vlan));

  lacp_handle_packet (vlan_skaddr.port, buf + sizeof (struct sockaddr_vlan),
                                        nbytes - sizeof (struct sockaddr_vlan) );

  return nbytes;
}

/* Client connection is established.  Client send service description
   message to the server.  */
int
lacp_client_connect (struct message_handler *mc, struct message_entry *me,
                     pal_sock_handle_t sock)
{
  int ret;
  struct preg_msg preg;

  /* Make the client socket blocking. */
  pal_sock_set_nonblocking (sock, PAL_FALSE);

  /* Register read thread.  */
  if ((struct lacp_client *)mc->info == &lacplink_async)
    message_client_read_register (mc);

  preg.len  = MESSAGE_REGMSG_SIZE;
  preg.value = HAL_SOCK_PROTO_LACP;

  /* Encode protocol identifier and send to HSL server */
  ret = pal_sock_write (sock, &preg, MESSAGE_REGMSG_SIZE);
  if (ret <= 0)
    {
      return RESULT_ERROR;
    }

  return RESULT_OK;

}

int
lacp_client_disconnect (struct message_handler *mc, struct message_entry *me,
                        pal_sock_handle_t sock)
{
  /* Stop message client.  */
  message_client_stop (mc);

  return RESULT_OK;
}

/* Make socket for netlink(RFC 3549) interface. */
int
lacp_client_create (struct lib_globals *zg, struct lacp_client *nl,
                    u_char async)
{
  struct message_handler *mc;
  int ret;

  /* Create async message client.  */
  mc = message_client_create (zg, MESSAGE_TYPE_ASYNC);
  if (mc == NULL)
    return RESULT_ERROR;

#ifdef HAVE_TCP_MESSAGE
  message_client_set_style_tcp (mc, HSL_ASYNC_PORT);
#else
  message_client_set_style_domain (mc, HSL_ASYNC_PATH);
#endif /* HAVE_TCP_MESSAGE */

  /* Initiate connection using lacp client connection manager.  */
  message_client_set_callback (mc, MESSAGE_EVENT_CONNECT,
                               lacp_client_connect);
  message_client_set_callback (mc, MESSAGE_EVENT_DISCONNECT,
                               lacp_client_disconnect);
  if (async)
    message_client_set_callback (mc, MESSAGE_EVENT_READ_MESSAGE,
        lacp_async_client_read);

  /* Link each other.  */
  mc->info = nl;
  nl->mc = mc;

  /* Start the lacp client. */
  ret = message_client_start (nl->mc);

  return ret;
}
#else
/* Send a frame over some LACP port. */

int
send_lacp (struct sockaddr_l2 *addr, unsigned char *data, int len)
{
  const char *fn_name = "send_lacp";

  if (lacp_instance->fwd_socket < 0)
    {
      zlog_warn (LACPM, "%s: lacp socket is not open", fn_name);
      return -1;
    }
  return pal_sock_sendto (lacp_instance->fwd_socket, data, len,
                          0, (struct pal_sockaddr *)addr,
                          sizeof (struct sockaddr_l2));
}
#endif /* HAVE_USER_HSL */

int
lacp_system_mac_address_set (u_char *addr)
{
  if (! addr)
    return RESULT_ERROR;

  pal_mem_cpy (LACP_SYSTEM_ID, addr, LACP_GRP_ADDR_LEN);
  SET_FLAG (lacp_instance->flags, LACP_SYSTEM_ID_ACTIVE);

  lacp_activate ();

#ifdef HAVE_HA
  lacp_cal_modify_instance ();
#endif /* HAVE_HA */

  return RESULT_OK;
}

/* Get System ID */
int
lacp_system_mac_address_get (u_char *system_id)
{
  pal_mem_cpy (system_id, LACP_SYSTEM_ID, LACP_GRP_ADDR_LEN);

  return RESULT_OK;
}

int
lacp_system_mac_address_unset ()
{
  pal_mem_set (LACP_SYSTEM_ID, 0, LACP_GRP_ADDR_LEN);
  UNSET_FLAG (lacp_instance->flags, LACP_SYSTEM_ID_ACTIVE);

  lacp_deactivate ();

#ifdef HAVE_HA
  lacp_cal_modify_instance ();
#endif /* HAVE_HA */

  return RESULT_OK;
}



int
lacp_system_mac_address_update ()
{
  struct route_node *rn;
  struct interface *ifp;

  for (rn = route_top (LACPM->ifg.if_table); rn; rn = route_next (rn))
    {
      if ((ifp = rn->info) == NULL || if_is_loopback (ifp))
        continue;

      lacp_system_mac_address_set (ifp->hw_addr);
      break;
    }

  return RESULT_OK;
}


void
lacp_activate ()
{
}


void
lacp_deactivate ()
{
}

#ifdef HAVE_SMI

/* Get index bitmap of aggregators configured */
int
lacp_get_aggregator_idx(struct smi_lacp_agg_bmp *aggBmp)
{
  register unsigned int agg_ndx;
  struct lacp_aggregator *agg = NULL;

  SMI_BMP_INIT (*aggBmp);

  for (agg_ndx = 0; agg_ndx < LACP_AGG_COUNT_MAX; agg_ndx++)
  {
    agg = LACP_AGG_LIST[agg_ndx];
    if(agg)
      SMI_BMP_SET (*aggBmp, agg->agg_ix);
  }
  return RESULT_OK;
}

/* Get detailed ether channel information */
int
lacp_get_aggregrator_detail(u_int32_t lacpAdminKey,
                            struct smi_lacp_channel *ch_detail)
{
  register unsigned int agg_ndx;
  unsigned int link_ndx;
  struct lacp_aggregator *agg = NULL;
  struct smi_lacp_link *link_detail = NULL;

  pal_mem_set(ch_detail, 0, sizeof(struct smi_lacp_channel));

  for (agg_ndx = 0; agg_ndx < LACP_AGG_COUNT_MAX; agg_ndx++)
  {
    agg = LACP_AGG_LIST[agg_ndx];
    if(agg && (agg->actor_admin_aggregator_key == lacpAdminKey))
    {
      ch_detail->linkCnt = agg->linkCnt;
      pal_mem_cpy(ch_detail->aggregator_mac_address,
                  agg->aggregator_mac_address, LACP_GRP_ADDR_LEN);
      ch_detail->actor_admin_aggregator_key = agg->actor_admin_aggregator_key;
      ch_detail->actor_oper_aggregator_key = agg->actor_oper_aggregator_key;
      ch_detail->actor_oper_aggregator_key = agg->actor_oper_aggregator_key;
      ch_detail->receive_state = agg->receive_state;
      ch_detail->transmit_state = agg->transmit_state;
      ch_detail->individual_aggregator = agg->individual_aggregator;
      ch_detail->ready = agg->ready;
      ch_detail->partner_system_priority = agg->partner_system_priority;
      pal_mem_cpy(ch_detail->partner_system,
                  agg->partner_system, LACP_GRP_ADDR_LEN);
      for (link_ndx = 0; link_ndx < LACP_MAX_AGGREGATOR_LINKS; link_ndx++)
        if(agg->link[link_ndx])
        {
          link_detail = &(ch_detail->lacp_link[link_ndx]);
          strncpy(link_detail->name, (agg->link[link_ndx]->name),
                  LACP_IFNAMSIZ);
          link_detail->actor_port_number =
                            agg->link[link_ndx]->actor_port_number;
          link_detail->sync_info =
          GET_SYNCHRONIZATION(agg->link[link_ndx]->actor_oper_port_state) &
          GET_SYNCHRONIZATION(agg->link[link_ndx]->partner_oper_port_state);
        }
      break;
    }
  }
  return RESULT_OK;
}

/* Get ether channel summary */
int
lacp_get_aggregrator_summary (u_int32_t lacpAdminKey,
                              struct smi_lacp_channel_summary *ch_summary)
{
  register unsigned int agg_ndx;
  unsigned int link_ndx;
  struct lacp_aggregator *agg = NULL;
  struct smi_lacp_link *link_detail = NULL;

  pal_mem_set(ch_summary, 0, sizeof(struct smi_lacp_channel_summary));

  for (agg_ndx = 0; agg_ndx < LACP_AGG_COUNT_MAX; agg_ndx++)
  {
    agg = LACP_AGG_LIST[agg_ndx];
    if(agg && (agg->actor_admin_aggregator_key == lacpAdminKey))
    {
      ch_summary->actor_admin_aggregator_key = agg->actor_admin_aggregator_key;
      ch_summary->actor_oper_aggregator_key = agg->actor_oper_aggregator_key;
      for (link_ndx = 0; link_ndx < LACP_MAX_AGGREGATOR_LINKS; link_ndx++)
        if(agg->link[link_ndx])
        {
          link_detail = &(ch_summary->lacp_link[link_ndx]);
          strncpy(link_detail->name, (agg->link[link_ndx]->name),
                  LACP_IFNAMSIZ);
          link_detail->actor_port_number =
                            agg->link[link_ndx]->actor_port_number;
          link_detail->sync_info =
          GET_SYNCHRONIZATION(agg->link[link_ndx]->actor_oper_port_state) &
          GET_SYNCHRONIZATION(agg->link[link_ndx]->partner_oper_port_state);
        }
      break;
    }
  }
  return RESULT_OK;
}

/* Get the packet traffic on all ports of a given LACP aggregator.*/
int
lacp_get_aggregator_port_counters (struct lacp_link *link,
                                   struct smi_lacp_link_counters *link_counters)
{
  if (link == NULL)
    return RESULT_ERROR;

  pal_mem_set(link_counters, 0, sizeof(struct smi_lacp_link_counters));

  pal_strncpy(link_counters->name, link->name, LACP_IFNAMSIZ);
  link_counters->lacpdu_sent_count = link->lacpdu_sent_count;
  link_counters->lacpdu_recv_count = link->lacpdu_recv_count;
  link_counters->mpdu_sent_count = link->mpdu_sent_count;
  link_counters->mpdu_recv_count = link->mpdu_recv_count;
  link_counters->pckt_sent_err_count = link->pckt_sent_err_count;
  link_counters->pckt_recv_err_count = link->pckt_recv_err_count;

  return RESULT_OK;
}

#endif /* HAVE_SMI */
