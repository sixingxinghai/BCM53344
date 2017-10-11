/* Copyright (C) 2002-2003  All Rights Reserved. */

/* MSTP SMI server implementation.  */

#include "smi_server.h"
#include "smi_mstp_server.h"

/* Initialize MSTP SMI server.  */
struct smi_server *
mstp_smi_server_init (struct lib_globals *zg)
{
  int ret;
  struct smi_server *as;
  struct message_handler *ms;

  /* Create message server.  */
  ms = message_server_create (zg);
  if (! ms)
    return NULL;

#ifndef HAVE_TCP_MESSAGE
  /* Set server type to UNIX domain socket.  */
  message_server_set_style_domain (ms, SMI_SERV_MSTP_PATH);
#else /* HAVE_TCP_MESSAGE */
  message_server_set_style_tcp (ms, SMI_PORT_MSTP);
#endif /* !HAVE_TCP_MESSAGE */

  /* Set call back functions.  */
  message_server_set_callback (ms, MESSAGE_EVENT_CONNECT,
                               smi_server_connect);
  message_server_set_callback (ms, MESSAGE_EVENT_DISCONNECT,
                               smi_server_disconnect);
  message_server_set_callback (ms, MESSAGE_EVENT_READ_MESSAGE,
                               smi_server_read_msg);

  /* Start SMI server.  */
  ret = message_server_start (ms);
  if (ret < 0)
    ;

  /* When message server works fine, go forward to create SMI server
     structure.  */
  as = XCALLOC (MTYPE_SMISERVER, sizeof (struct smi_server));
  as->zg = zg;
  as->ms = ms;
  ms->info = as;

  /* Set version and protocol.  */
  smi_server_set_version (as, SMI_PROTOCOL_VERSION_1);
  smi_server_set_protocol (as, SMI_PROTO_SMISERVER);

  /* Set callback functions to SMI.  */

  smi_server_set_callback (as, SMI_MSG_MSTP_ADDINSTANCE,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_DELINSTANCE,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SETAGE,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GETAGE,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_ADDPORT,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_DELPORT,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SETHELLOTIME,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GETHELLOTIME,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SETMAXAGE,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GETMAXAGE,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SETPORTEDGE,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GETPORTEDGE,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SETVERSION,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GETVERSION,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SETPR,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GETPR,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SETFWDD,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GETFWDD,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SETMPR,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GETMPR,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
 smi_server_set_callback (as, SMI_MSG_MSTP_SETCOST,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
 smi_server_set_callback (as, SMI_MSG_MSTP_GETCOST,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
 smi_server_set_callback (as, SMI_MSG_MSTP_SETRSTROLE,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
 smi_server_set_callback (as, SMI_MSG_MSTP_GETRSTROLE,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
 smi_server_set_callback (as, SMI_MSG_MSTP_SETRSTTCN,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
 smi_server_set_callback (as, SMI_MSG_MSTP_GETRSTTCN,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SETPORT_HELLOTIME,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GETPORT_HELLOTIME,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SETP2P,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GETP2P,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SETPORT_PATHCOST,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GETPORT_PATHCOST,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SETMAXHOPS,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GETMAXHOPS,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SETPORT_PRIORITY,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GETPORT_PRIORITY,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SETPORT_RESTRICT,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GETPORT_RESTRICT,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SETPORT_RESTRICTTCN,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GETPORT_RESTRICTTCN,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SETPORT_ROOTGUARD,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GETPORT_ROOTGUARD,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SETPORT_BPDUFILTER,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GETPORT_BPDUFILTER,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_ENABLE_BRIDGE,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_DISABLE_BRIDGE,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SETPORT_BPDUGUARD,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GETPORT_BPDUGUARD,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SET_TXHOLDCOUNT,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GET_TXHOLDCOUNT,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SETBRIDGE_BPDUGUARD,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GETBRIDGE_BPDUGUARD,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SETBRIDGE_TIMEOUTEN,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GETBRIDGE_TIMEOUTEN,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SETBRIDGE_TIMEOUTINT,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GETBRIDGE_TIMEOUTINT,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SETMSTI_PORTPRIORITY,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GETMSTI_PORTPRIORITY,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SETREVISION_NUMBER,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GETREVISION_NUMBER,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SETAUTOEDGE,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GETAUTOEDGE,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SETREGIONNAME,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GETREGIONNAME,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GET_SPANNING_DETAILS,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GET_SPANNING_INTERFACE,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GET_SPANNING_MST,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GET_SPANNING_MST_CONF,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_STP_MSTDETAIL,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_STP_MSTDETAIL_IF,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_SETBRIDGE_BPDUFILTER,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_GETBRIDGE_BPDUFILTER,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_CHECK,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_GET_BRIDGE_STATUS,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
  smi_server_set_callback (as, SMI_MSG_MSTP_HA_SWITCH,
                           smi_parse_mstp,
                           mstp_smi_server_recv);
 
  return as;
}

/* Send response back to SMI clients */
int
mstp_smi_server_send_sync_message(struct smi_server_entry *ase, 
                                  int msgtype, int status, 
                                  void *getmsg)
{
  int len = 0;

  if(msgtype == SMI_MSG_STATUS)
    {
      struct smi_msg_status smsg;
      if(status == 0) /* success */
        {
          smsg.status_code = SMI_STATUS_SUCCESS;
          sprintf(smsg.reason, "Value set successfully");
        } else {
          smsg.status_code = SMI_STATUS_FAILURE;
          sprintf(smsg.reason, "Error in setting value");
        }
        /* Send the success/failure back to client */
        /* Set encode pointer and size.  */
        ase->send.pnt = ase->send.buf + SMI_MSG_HEADER_SIZE;
        ase->send.size = ase->send.len - SMI_MSG_HEADER_SIZE;
  
        /* Encode SMI if msg.  */
        len = smi_encode_statusmsg (&ase->send.pnt, &ase->send.size, &smsg);
        if (len < 0)
          return len;
        /* Send it to client.  */
        smi_server_send_message (ase, 0, 0, SMI_MSG_STATUS, 0, len);
     } else
     {
       struct smi_msg_mstp *mstpmsg = (struct smi_msg_mstp *) getmsg;

       /* Send the msg back to client */
       ase->send.pnt = ase->send.buf + SMI_MSG_HEADER_SIZE;
       ase->send.size = ase->send.len - SMI_MSG_HEADER_SIZE;

       /* Encode mstp msg.  */
       len = smi_encode_mstpmsg (&ase->send.pnt, &ase->send.size, mstpmsg);
       if (len < 0)
         return len;
       smi_server_send_message (ase, 0, 0, msgtype, 0, len);
     }
  return 0;
}
