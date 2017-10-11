/* Copyright (C) 2003  All Rights Reserved.  */

#include "pal.h"
#include "lib.h"
#include "bfd_client.h"
#include "bfd_client_api.h"
#include "mplsonm.h"

#ifdef HAVE_MPLS_OAM 
void
mpls_onm_disconnect_process (struct bfd_client_api_server *api_server)
{
  printf ("OAM Disconnected \n");
  pal_exit(0);
}

void
mplsonm_oam_init (struct lib_globals * zg)
{
  struct bfd_client_api_callbacks cb;
  memset (&cb, 0, sizeof (struct bfd_client_api_callbacks));

  cb.disconnected = mpls_onm_disconnect_process;
  cb.oam_ping_response = mpls_onm_ping_response;
  cb.oam_trace_response = mpls_onm_trace_response;
  cb.oam_error = mpls_onm_error;

  bfd_client_api_oam_client_new (zg, &cb);
  bfd_client_api_client_start (zg);
}

int
mpls_onm_error (struct bfd_msg_header *header,
                void *arg, void *message)
{
  struct oam_msg_mpls_ping_error *msg;
  msg = message;

 /* Update only for last sent packet */
  if (msg->recv_count > onm_data.recv_count)
    onm_data.recv_count = msg->recv_count;
  onm_data.resp_count++;

  switch (msg->msg_type)
    {
      case OAM_MPLSONM_ECHO_TIMEOUT:
        printf (".\n");
        break;
      
      case OAM_MPLSONM_NO_FTN:
        printf (" Q  FTN Entry Not Found\n");
        pal_exit (0);
        break;

      case OAM_MPLSONM_TRACE_LOOP:
        printf (" Traceroute Aborted same address detected \n");
        pal_exit (0);
        break;
      
      case OAM_MPLSONM_PACKET_SEND_ERROR:
        printf ("Q OAM Packet send failed\n");
        pal_exit (0);
        break;
      
      case OAM_MPLSONM_ERR_EXPLICIT_NULL:
        printf ("Q Cannot ping LSP to directly connected LSR : PHP in use\n");
        pal_exit (0);
        break;
      
      case OAM_MPLSONM_ERR_NO_CONFIG:
        printf ("Q VC or VPN not configured\n");
        pal_exit (0);
        break;

#ifdef HAVE_VCCV
      case OAM_MPLSONM_ERR_VCCV_NOT_IN_USE:
        printf ("VCCV not in use\n");
        pal_exit (0);
        break;
#endif /* HAVE_VCCV */

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
#endif /* HAVE_MPLS_OAM */
