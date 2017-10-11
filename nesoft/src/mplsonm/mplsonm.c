/* Copyright (C) 2006  All Rights Reserved.  */

#include "pal.h"
#include "lib.h"
#include "lib/linklist.h"
#include "stream.h"
#include "thread.h"
#include "log.h"
#include "table.h"
#include "tlv.h"
#include "mplsonm.h"
#include "oam_mpls_msg.h"

extern struct mpls_onm_cmd_options mpls_onm_options[];
#define PRINT_ADDR(ADDR) \
{ \
  pal_mem_set (addr_buf, 0, 5); \
  pal_mem_cpy (addr_buf, &ADDR, 4); \
  printf ("%d.%d.%d.%d",addr_buf[0], \
           addr_buf[1], addr_buf[2], addr_buf[3]); \
}

#define PRINT_RTT(RX_SECS, RX_USECS, TX_SECS, TX_USECS) \
{ \
   u_int32_t secs; \
   float usecs; \
   float rtt; \
\
    /* Get RTT */ \
  secs = ((float)(RX_SECS) - (float)(TX_SECS)) * 1000; \
  usecs = ((float)(RX_USECS) - (float)(TX_USECS)) / 1000; \
  rtt = (float) (secs  + usecs); \
  printf (" %0.2f ms", rtt); \
  if ((onm_data.rtt_min > 0) && (onm_data.rtt_max > 0)) \
   { \
    if (onm_data.rtt_min > rtt) \
       onm_data.rtt_min = rtt; \
    if (onm_data.rtt_max < rtt) \
       onm_data.rtt_max = rtt; \
   } \
    else \
   { \
       onm_data.rtt_max = rtt; \
       onm_data.rtt_min = rtt; \
   } \
  onm_data.rtt_avg = (onm_data.rtt_min + onm_data.rtt_max)/2; \
}

#ifdef HAVE_MPLS_OAM
#define PRINT_FEC() \
{\
  if (onm_data.mpls_oam.type == MPLSONM_OPTION_RSVP) \
    { \
      if (MSG_CHECK_CTYPE (onm_data.mpls_oam.cindex,  \
                      OAM_CTYPE_MSG_MPLSONM_OPTION_TUNNELNAME)) \
        printf (" %s ", onm_data.mpls_oam.u.rsvp.tunnel_name); \
      else \
        PRINT_ADDR (onm_data.fec_addr); \
    } \
 else if (onm_data.mpls_oam.type == MPLSONM_OPTION_L2CIRCUIT) \
    { \
      printf ("VC Id : %d", onm_data.mpls_oam.u.l2_data.vc_id); \
    } \
  else \
    PRINT_ADDR (onm_data.fec_addr); \
}
#else /* HAVE_NSM_MPLS_OAM */
#define PRINT_FEC() \
{\
  if (onm_data.mpls_oam.type == MPLSONM_OPTION_RSVP) \
    { \
      if (NSM_CHECK_CTYPE (onm_data.mpls_oam.cindex,  \
                      NSM_CTYPE_MSG_MPLSONM_OPTION_TUNNELNAME)) \
        printf (" %s ", onm_data.mpls_oam.u.rsvp.tunnel_name); \
      else \
        PRINT_ADDR (onm_data.fec_addr); \
    } \
 else if (onm_data.mpls_oam.type == MPLSONM_OPTION_L2CIRCUIT) \
    { \
      printf ("VC Id : %d", onm_data.mpls_oam.u.l2_data.vc_id); \
    } \
  else \
    PRINT_ADDR (onm_data.fec_addr); \
}
#endif /* HAVE_MPLS_OAM */

char 
mplsonm_code_to_str (u_int32_t code)
{
  switch (code)
    {
    case MPLS_OAM_DEFAULT_RET_CODE:
      return 'x';
    case MPLS_OAM_MALFORMED_REQUEST:
      return 'M';
    case MPLS_OAM_ERRORED_TLV:
      return 'm';
    case MPLS_OAM_RTR_IS_EGRESS_AT_DEPTH:
      return '!';
    case MPLS_OAM_RTR_HAS_NO_MAPPING_AT_DEPTH:
      return 'N';
    case MPLS_OAM_DSMAP_MISMATCH:
      return 'D';
    case MPLS_OAM_UPSTREAM_IF_UNKNOWN:
      return 'U';
    case MPLS_OAM_LBL_SWITCH_AT_DEPTH:
      return 'R';
    case MPLS_OAM_LBL_SWITCH_IP_FWD_AT_DEPTH:
      return 'B';
    case MPLS_OAM_MAPPING_ERR_AT_DEPTH:
      return 'f';
    case MPLS_OAM_NO_MAPPING_AT_DEPTH:
      return 'F';
    case MPLS_OAM_PROTOCOL_ERR_AT_DEPTH:
      return 'P';
    default:
      return code;
   }
}

#ifdef HAVE_MPLS_OAM
int
mpls_onm_ping_response (struct bfd_msg_header *header,
                        void *arg, void *message)
{ 
  struct oam_msg_mpls_ping_reply *msg;
#else /* HAVE_NSM_MPLS_OAM */
int
mpls_onm_ping_response (struct nsm_msg_header *header,
                        void *arg, void *message)
{ 
  struct nsm_msg_mpls_ping_reply *msg;
#endif /* HAVE_MPLS_OAM */
  u_char addr_buf[5];

  msg = message;

  /* Update only for last sent packet */
  if (msg->recv_count > onm_data.recv_count)
    onm_data.recv_count = msg->recv_count;
  onm_data.resp_count++;

  if (msg->ret_code == MPLS_OAM_RTR_IS_EGRESS_AT_DEPTH)
    onm_data.reply_count++;


  if (mpls_oam.verbose)
    {
      printf ("%c ", mplsonm_code_to_str (msg->ret_code));
      printf ("seq_num = %d ", msg->ping_count);
      PRINT_ADDR(msg->reply);

      PRINT_RTT (msg->recv_time_sec, msg->recv_time_usec, msg->sent_time_sec, 
                 msg->sent_time_usec);
      if (msg->ret_code != MPLS_OAM_RTR_IS_EGRESS_AT_DEPTH)
        printf (" return code %d",  msg->ret_code);
      printf ("\n");
    }
   else
    {
      printf (" %c", mplsonm_code_to_str (msg->ret_code));
      printf ("\n");
      fflush(stdout);
    }
  if (onm_data.resp_count >= onm_data.mpls_oam.repeat)
    {
      printf ("\n");
      mpls_onm_disp_summary();
      pal_exit(0);
    }

    return 0;
}

#ifdef HAVE_MPLS_OAM
int
mpls_onm_trace_response (struct bfd_msg_header *header,
                         void *arg, void *message)
{ 
  struct oam_msg_mpls_tracert_reply *msg;
#else /* HAVE_NSM_MPLS_OAM */
int
mpls_onm_trace_response (struct nsm_msg_header *header,
                         void *arg, void *message)
{ 
  struct nsm_msg_mpls_tracert_reply *msg;
#endif /* HAVE_MPLS_OAM */
  u_char addr_buf[5];
  struct shimhdr *label;
  struct  listnode *node;
  msg = message;
  u_char cnt=0;

 /* Update only for last sent packet */
  if (msg->recv_count > onm_data.recv_count)
    onm_data.recv_count = msg->recv_count;

  if (msg->recv_count)
    {
      onm_data.resp_count++;
      printf ("%c ", mplsonm_code_to_str (msg->ret_code));
    }
  else
    printf ("  ");

  onm_data.reply_count++;

  if (mpls_oam.verbose)
    {
      printf ("%d ", onm_data.resp_count);
      PRINT_ADDR (msg->reply);
      if (!msg->last)
        {
          printf (" [Labels: ");
          LIST_LOOP (msg->ds_label, label, node)
            {
              if (cnt)
                printf ("/");
              if (get_label_net (label->shim) == LABEL_IMPLICIT_NULL)
                printf ("implicit-null");
              else
                printf (" %d", get_label_net (label->shim)); 
              cnt++;
            }
 
          if ((!msg->recv_count) && 
               (onm_data.mpls_oam.type != MPLSONM_OPTION_L2CIRCUIT))
            {
               if (get_label_net (msg->out_label) == LABEL_IMPLICIT_NULL)
                 printf ("implicit-null");
               else
                 printf (" %d", get_label_net(msg->out_label));
            }
          printf ("]");
          }
        if (msg->recv_count)
          {
            PRINT_RTT (msg->recv_time_sec, msg->recv_time_usec, msg->sent_time_sec,
                       msg->sent_time_usec);
          }
      }
  printf ("\n");
  if (msg->last)
    {
      printf ("\n");
      pal_exit(0);
    }
  return 0;
}

void mpls_onm_disp_summary (void)
{
  float rate = 0;
  if ((mpls_oam.req_type != MPLSONM_OPTION_PING) ||
       (!mpls_oam.verbose))
    {
      printf ("\n");
      return;
    }
  if (onm_data.recv_count)
    rate = ((float)onm_data.reply_count/(float)onm_data.recv_count);

  printf ("Success Rate is %.2f percent (%d/%d)\n", 
           rate*100, 
           onm_data.reply_count,
           onm_data.recv_count);
  printf ("round-trip min/avg/max = %.2f/%.2f/%.2f \n",
           onm_data.rtt_min,
           onm_data.rtt_avg,
           onm_data.rtt_max);
}

void mpls_onm_detail (void)
{
  u_char addr_buf[5];
  if (mpls_oam.req_type == MPLSONM_OPTION_PING)
  {
     printf ("Sending %d MPLS Echos to ", onm_data.mpls_oam.repeat);
  }
  else
  {
     printf ("Tracing MPLS Label Switched Path to ");
  }
  PRINT_FEC();
  printf (", timeout is %d seconds", onm_data.mpls_oam.timeout);
  printf ("\n\n");
  printf ("Codes: \n'!' - Success, 'Q' - request not sent, '.' - timeout, \n"\
          "'x' - Retcode 0, 'M' - Malformed Request, 'm' - Errored TLV, \n"\
          "'N' - LBL Mapping Err, 'D' - DS Mismatch,\n"\
          "'U' - Unknown Interface, 'R' - Transit (LBL Switched),\n" \
          "'B' - IP Forwarded, 'F' No FEC Found, 'f' - FEC Mismatch,\n" \
          "'P' - Protocol Error, 'X' - Unknown code\n");
  printf ("\n Type 'Ctrl+C' to abort");
  printf ("\n\n");
}
