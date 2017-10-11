/* Copyright (C) 2003  All Rights Reserved.  */

#include "pal.h"
#include "lib.h"
#include "thread.h"
#include "log.h"
#include "table.h"
#include "nsm_client.h"
#include "tlv.h"
#include "mplsonm.h"

#ifdef HAVE_NSM_MPLS_OAM 
extern struct mpls_onm_cmd_options mpls_onm_options[];

int
mpls_onm_nsm_disconnect_process (void)
{
  printf ("NSM Disconnected \n");
  pal_exit(0);
  return 0;
}

void
mplsonm_nsm_init (struct lib_globals * zg)
{
  struct nsm_client * nc;

  nc = nsm_client_create (zg, 0);
  if (!nc)
    return;

  /* Set version and protocol.  */
  nsm_client_set_version (nc, NSM_PROTOCOL_VERSION_1);
  nsm_client_set_protocol (nc, APN_PROTO_MPLSONM);

  nsm_client_set_service (nc, NSM_SERVICE_INTERFACE);
  nsm_client_set_callback (nc, NSM_MSG_MPLS_PING_REPLY,
                           mpls_onm_ping_response);
  nsm_client_set_callback (nc, NSM_MSG_MPLS_TRACERT_REPLY,
                           mpls_onm_trace_response);
  nsm_client_set_callback (nc, NSM_MSG_MPLS_OAM_ERROR,
                          mpls_onm_error);

  /* Set disconnect handling callback */
  nsm_client_set_disconnect_callback (nc, mpls_onm_nsm_disconnect_process);

  nsm_client_start (nc);
}

int
mpls_onm_error (struct nsm_msg_header *header,
                void *arg, void *message)
{
  struct nsm_msg_mpls_ping_error *msg;
  msg = message;

 /* Update only for last sent packet */
  if (msg->recv_count > onm_data.recv_count)
    onm_data.recv_count = msg->recv_count;
  onm_data.resp_count++;

  switch (msg->msg_type)
    {
      case NSM_MPLSONM_ECHO_TIMEOUT:
        printf (".\n");
        break;
      
      case NSM_MPLSONM_NO_FTN:
        printf (" Q  FTN Entry Not Found\n");
        pal_exit (0);
        break;

      case NSM_MPLSONM_TRACE_LOOP:
        printf (" Traceroute Aborted same address detected \n");
        pal_exit (0);
        break;
      
      case NSM_MPLSONM_PACKET_SEND_ERROR:
        printf ("Q OAM Packet send failed\n");
        pal_exit (0);
        break;
      
      case NSM_MPLSONM_ERR_EXPLICIT_NULL:
        printf ("Q Cannot ping LSP to directly connected LSR : PHP in use\n");
        pal_exit (0);
        break;
      
      case NSM_MPLSONM_ERR_NO_CONFIG:
        printf ("Q VC or VPN not configured\n");
        pal_exit (0);
        break;

      default:
        printf (" Ping/Trace Failed \n");
        pal_exit (0);
        break;
    }

  if (onm_data.mpls_oam.req_type == MPLSONM_OPTION_PING)
    {
      if (onm_data.resp_count >= onm_data.mpls_oam.repeat)
        {
          mpls_onm_disp_summary();
          pal_exit(0);
        }
       return 0;
    }
  else 
    {
      if (onm_data.resp_count >= onm_data.mpls_oam.ttl)
        {
          mpls_onm_disp_summary();
          printf ("\n");
          pal_exit(0);
        }
    }

  return 0;
}

#ifdef HAVE_MPLS_OAM_ITUT

void
mplsonm_nsm_itut_init (struct lib_globals * zg)
{
  struct nsm_client * nc;

  nc = nsm_client_create (zg, 0);
  if (!nc)
    return;

  /* Set version and protocol.  */
  nsm_client_set_version (nc, NSM_PROTOCOL_VERSION_2);
  nsm_client_set_protocol (nc, APN_PROTO_ITUTMPLSONM);

  nsm_client_set_service (nc, NSM_SERVICE_INTERFACE);

  nsm_client_start (nc);
}

#endif /*HAVE_MPLS_OAM_ITUT*/
#endif /* HAVE_NSM_MPLS_OAM */
