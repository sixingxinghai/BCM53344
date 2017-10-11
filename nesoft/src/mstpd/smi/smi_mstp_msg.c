/* Copyright (C) 2002-2003  All Rights Reserved. */

/* MSTP SMI implementation.  */

#include "smi_server.h"
#include "smi_mstp_server.h"

/* Set/Get MSTP params */
int
mstp_smi_server_recv(struct smi_msg_header *header,
                     void *arg, void *message)
{
  struct smi_server_entry *ase = NULL;
  struct smi_server *as = NULL;
  struct smi_msg_mstp *msg = NULL, replymsg;
  int status = 0, ret = 0;
  int msgtype = header->type;
  struct interface *ifp = NULL;

  SMI_FN_ENTER ();

  ase = arg;
  as = ase->as;

  msg = (struct smi_msg_mstp *)message;

  smi_mstp_dump(mstpm, msg);

  /* Initialize cindices */  
  replymsg.cindex = 0;
  replymsg.extended_cindex_1 = 0;

  #ifdef HAVE_HA
  if ((HA_IS_STANDBY (mstpm)))
  {
    ret = mstp_check_func_type (msgtype);
    if (ret == SMI_SET_FUNC)
      SMI_FN_EXIT (mstp_smi_server_send_sync_message (ase,
                                                      SMI_MSG_STATUS,
                                                      SMI_ERROR, NULL));
  }
  #endif /* HAVE_HA */

  switch(header->type)
    {
      case SMI_MSG_MSTP_ADDINSTANCE:
      {
        u_int16_t vid;
        int local_status = 0;

        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_EXTENDED);
        SMI_SET_CTYPE (replymsg.extended_cindex_1, SMI_MSTP_CTYPE_VLAN_BMP);

        SMI_BMP_INIT (replymsg.vlanBmp);

        for (vid = SMI_VLAN_ID_START; vid <= SMI_VLAN_ID_END; vid++)
        {
          if (!SMI_BMP_IS_MEMBER (msg->vlanBmp, vid))
            continue;

          status = mstp_api_add_instance (msg->bridge_name, msg->instance, vid);
          if (status < 0)
          {
            local_status = SMI_ERROR;
            continue;
          }

          SMI_BMP_SET(replymsg.vlanBmp, vid);
        }

        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_MSTP_ADDINSTANCE,
                                                 local_status, &replymsg);
      }
      break;

      case SMI_MSG_MSTP_DELINSTANCE:
      {
        u_int16_t vid;
        int local_status = 0;

        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_EXTENDED);
        SMI_SET_CTYPE (replymsg.extended_cindex_1, SMI_MSTP_CTYPE_VLAN_BMP);

        SMI_BMP_INIT (replymsg.vlanBmp);

        for (vid = SMI_VLAN_ID_START; vid <= SMI_VLAN_ID_END; vid++)
        {
          if (!SMI_BMP_IS_MEMBER (msg->vlanBmp, vid))
            continue;

          status = mstp_api_delete_instance_vlan (msg->bridge_name, 
                                                  msg->instance, vid);
          if (status < 0)
          {
            local_status = SMI_ERROR;
            continue;
          }

          SMI_BMP_SET(replymsg.vlanBmp, vid);
        }

        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_MSTP_DELINSTANCE,
                                                 local_status, &replymsg);
      }
      break;

      case SMI_MSG_MSTP_SETAGE:
        status = mstp_api_set_ageing_time(msg->bridge_name, msg->age);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
        break;

      case SMI_MSG_MSTP_GETAGE:

        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_AGE);

        status = mstp_api_get_ageing_time (msg->bridge_name, 
                                           &(replymsg.age));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;
                                                                                
        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
        break;

      case SMI_MSG_MSTP_ADDPORT:
        status = mstp_api_add_port(msg->bridge_name, msg->if_name, 
                                   MSTP_VLAN_NULL_VID, msg->instance,
                                   PAL_FALSE);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_MSTP_DELPORT:
        status = mstp_api_delete_port(msg->bridge_name, msg->if_name, 
                                      msg->instance, MSTP_VLAN_NULL_VID, 
                                      MSTP_PORT_DEL_FORCE, MSTP_NOTIFY_FWD);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_MSTP_SETHELLOTIME:
        status = mstp_api_set_hello_time(msg->bridge_name, msg->hello_time);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
        break;

      case SMI_MSG_MSTP_GETHELLOTIME:
        /* Bridge hellotime TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_HELLOTIME);

        status = mstp_api_get_hello_time (msg->bridge_name, 
                                          &(replymsg.hello_time));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;
                                                                                
        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
        break;

      case SMI_MSG_MSTP_SETMAXAGE:
        status = mstp_api_set_max_age(msg->bridge_name, msg->max_age);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
        break;

      case SMI_MSG_MSTP_GETMAXAGE:
        /* Max age TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_MAXAGE);

        status = mstp_api_get_max_age (msg->bridge_name, &(replymsg.max_age));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;
                                                                                
        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
        break;

      case SMI_MSG_MSTP_SETPORTEDGE:
        status = mstp_api_set_port_edge(msg->bridge_name, msg->if_name, 
                                        msg->port_edge, SMI_MSTP_NOPORTFAST);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
        break;

      case SMI_MSG_MSTP_GETPORTEDGE:
        /* Port edge TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_PORTEDGE);
        status = mstp_api_get_port_edge (msg->bridge_name, msg->if_name,
                                         &(replymsg.port_edge));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_MSTP_GETPORTEDGE;

        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
        break;

      case SMI_MSG_MSTP_SETVERSION:
        status = mstp_api_set_port_forceversion(msg->bridge_name, msg->if_name, 
                                                msg->version);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
        break;

      case SMI_MSG_MSTP_GETVERSION:
        /* Version TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_VERSION);

        status = mstp_api_get_port_forceversion(msg->bridge_name, msg->if_name,
                                                &(replymsg.version));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;
                                                                                
        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
        break;

      case SMI_MSG_MSTP_SETPR:
        status = mstp_api_set_bridge_priority(msg->bridge_name, 
                                              msg->new_priority);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_MSTP_GETPR:
        /* Priority TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_PRIORITY);

        status = mstp_api_get_bridge_priority(msg->bridge_name, 
                                              &(replymsg.new_priority));
        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;
                                                                                
        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;

      case SMI_MSG_MSTP_SETFWDD:
        status = mstp_api_set_forward_delay(msg->bridge_name,
                                            msg->forward_delay);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_MSTP_GETFWDD:
        /* Forward delay TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_FWDDELAY);

        status = mstp_api_get_forward_delay (msg->bridge_name, 
                                            &(replymsg.forward_delay));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;
                                                                                
        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;

      case SMI_MSG_MSTP_SETMPR:
        status = mstp_api_set_msti_bridge_priority(msg->bridge_name,
                                                   msg->instance, 
                                                   msg->msti_priority);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_MSTP_GETMPR:
        /* Priority TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_MSTIPRIORITY);

        status = mstp_api_get_msti_bridge_priority(msg->bridge_name,
                                                   msg->instance, 
                                                   &(replymsg.msti_priority));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;
                                                                                
        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;

      case SMI_MSG_MSTP_SETCOST:
        status = mstp_api_set_msti_port_path_cost(msg->bridge_name,
                                                  msg->if_name, msg->instance, 
                                                  msg->cost);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_MSTP_GETCOST:
        /* Pathcost TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_COST);

        status = mstp_api_get_msti_port_path_cost(msg->bridge_name,
                                                  msg->if_name, msg->instance,
                                                  &(replymsg.cost));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;
                                                                                
        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;  

      case SMI_MSG_MSTP_SETRSTROLE:
        status = mstp_api_set_msti_instance_restricted_role(msg->bridge_name,
                                                            msg->if_name, 
                                                            msg->instance, 
                                                          msg->restricted_role);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_MSTP_GETRSTROLE:
        /* Restricted Role  TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_RST);

        status = mstp_api_get_msti_instance_restricted_role(msg->bridge_name,
                                                    msg->if_name,
                                                    msg->instance,
                                                   &(replymsg.restricted_role));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;
                                                                                
        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;

      case SMI_MSG_MSTP_SETRSTTCN:
        status = mstp_api_set_msti_instance_restricted_tcn(msg->bridge_name,
                                                           msg->if_name, 
                                                           msg->instance, 
                                                           msg->restricted_tcn);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_MSTP_GETRSTTCN:
        /* Restricted TCN TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_RSTCN);

        status = mstp_api_get_msti_instance_restricted_tcn(msg->bridge_name,
                                                    msg->if_name,
                                                    msg->instance,
                                                    &(replymsg.restricted_tcn));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;
                                                                                
        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;

      case SMI_MSG_MSTP_SETPORT_HELLOTIME:
        status = mstp_api_set_port_hello_time(msg->bridge_name, msg->if_name, 
                                              msg->port_hello_time);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_MSTP_GETPORT_HELLOTIME:
        /* Bridge Hellotime  TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_PORTHELLO);

        status = mstp_api_get_port_hello_time(msg->bridge_name, msg->if_name,
                                              &(replymsg.port_hello_time));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;
                                                                                
        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;

      case SMI_MSG_MSTP_SETP2P:
        status = mstp_api_set_port_p2p(msg->bridge_name, msg->if_name, 
                                       msg->is_p2p);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_MSTP_GETP2P:
        /* Port P2P TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_PORTP2P);

        status = mstp_api_get_port_p2p(msg->bridge_name, msg->if_name,
                                       &(replymsg.is_p2p));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;
                                                                                
        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;

      case SMI_MSG_MSTP_SETPORT_PATHCOST:
        status = mstp_api_set_port_path_cost(msg->bridge_name, msg->if_name, 
                                             MSTP_VLAN_NULL_VID, msg->path_cost);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_MSTP_GETPORT_PATHCOST:
        /* Port Pathcost TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_PPATHCOST);

        status = mstp_api_get_port_path_cost(msg->bridge_name, msg->if_name,
                                             MSTP_VLAN_NULL_VID, 
                                             &(replymsg.path_cost));
                                                                                
        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;
                                                                                
        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;

      case SMI_MSG_MSTP_SETMAXHOPS:
          status = mstp_api_set_max_hops(msg->bridge_name,msg->max_hops);
          ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                   NULL);
      break;

      case SMI_MSG_MSTP_GETMAXHOPS:
        /* Max Hops TLV, Extended Ctype */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_EXTENDED);
        SMI_SET_CTYPE (replymsg.extended_cindex_1, SMI_MSTP_CTYPE_MAXHOPS);

        status = mstp_api_get_max_hops(msg->bridge_name, &(replymsg.max_hops));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;
                                                                                
        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;

      case SMI_MSG_MSTP_SETPORT_PRIORITY:
        status = mstp_api_set_port_priority(msg->bridge_name, msg->if_name, 
                                            MSTP_VLAN_NULL_VID, 
                                            msg->port_priority);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_MSTP_GETPORT_PRIORITY:
        /* Port Priority TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_PORTPRIORITY);

        status = mstp_api_get_port_priority(msg->bridge_name, msg->if_name,
                                            MSTP_VLAN_NULL_VID, 
                                            &(replymsg.port_priority));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;

        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;

     case SMI_MSG_MSTP_SETPORT_RESTRICT:
        status = mstp_api_set_port_restricted_role(msg->bridge_name, 
                                                   msg->if_name,
                                                   msg->restricted_prole);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_MSTP_GETPORT_RESTRICT:
        /* Port Restricted role TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_PORTRSTROLE);
  
        status = mstp_api_get_port_restricted_role(msg->bridge_name, 
                                                  msg->if_name,
                                                  &(replymsg.restricted_prole));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;

        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;
 
      case SMI_MSG_MSTP_SETPORT_RESTRICTTCN:
        status = mstp_api_set_port_restricted_tcn(msg->bridge_name, 
                                                  msg->if_name,
                                                  msg->restricted_ptcn);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_MSTP_GETPORT_RESTRICTTCN:
        /* Port Restricted TCN TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_PORTRSTTCN);

        status = mstp_api_get_port_restricted_tcn(msg->bridge_name,
                                                  msg->if_name,
                                                  &(replymsg.restricted_ptcn));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;

        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;

      case SMI_MSG_MSTP_SETPORT_ROOTGUARD:
        status = mstp_api_set_port_rootguard(msg->bridge_name, msg->if_name,
                                             msg->port_rootg);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_MSTP_GETPORT_ROOTGUARD:
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_PORTROOTG);

        status = mstp_api_get_port_rootguard(msg->bridge_name, msg->if_name,
                                             &(replymsg.port_rootg));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;

        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;

      case SMI_MSG_MSTP_SETPORT_BPDUFILTER:
        status = mstp_api_set_port_bpdufilter(msg->bridge_name, msg->if_name,
                                              msg->portfast_bpdu);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_MSTP_GETPORT_BPDUFILTER:
        /* Portfast bpdufilter TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_PORTBPDUF);
     
        status = mstp_api_get_port_bpdufilter(msg->bridge_name, msg->if_name,
                                              &(replymsg.portfast_bpdu));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;

        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;

      case SMI_MSG_MSTP_ENABLE_BRIDGE:
      {
        status = mstp_api_enable_bridge(msg->bridge_name, SMI_DONT_CHK_TYPE);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      }
      break;

      case SMI_MSG_MSTP_DISABLE_BRIDGE:
      {
        status = mstp_api_disable_bridge(msg->bridge_name, SMI_DONT_CHK_TYPE, 
                                         msg->br_forward);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      }
      break;

      case SMI_MSG_MSTP_SETPORT_BPDUGUARD:
        status = mstp_api_set_port_bpduguard(msg->bridge_name, msg->if_name,
                                             msg->bpdu_guard);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_MSTP_GETPORT_BPDUGUARD:
        /* Portfast bpduguard TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_BPDUGUARD);

        status = mstp_api_get_port_bpduguard(msg->bridge_name, msg->if_name,
                                             &(replymsg.bpdu_guard));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;

        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;

      case SMI_MSG_MSTP_SET_TXHOLDCOUNT:
        status = mstp_api_set_transmit_hold_count(msg->bridge_name,
                                                  msg->txholdcount);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_MSTP_GET_TXHOLDCOUNT:
        /*TXHOLDCOUNT  TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_TXHOLDCOUNT);

        status = mstp_api_get_transmit_hold_count(msg->bridge_name,
                                                  &(replymsg.txholdcount));
        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;

        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;

      case SMI_MSG_MSTP_SETBRIDGE_BPDUGUARD:
        status = mstp_api_set_bridge_portfast_bpduguard(msg->bridge_name,
                                                        msg->bpduguard_en);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_MSTP_GETBRIDGE_BPDUGUARD:
        /* Bridge Portfast bpduguard TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_BRIDGEBPDUG);

        status = mstp_api_get_bridge_portfast_bpduguard(msg->bridge_name,
                                                      &(replymsg.bpduguard_en));
        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;

        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;

      case SMI_MSG_MSTP_SETBRIDGE_TIMEOUTEN:
        status = mstp_api_set_bridge_errdisable_timeout_enable(msg->bridge_name,
                                                               msg->enabled);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_MSTP_GETBRIDGE_TIMEOUTEN:
        /*Errdisable timeout TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_TIMEOUTEN);

        status = mstp_api_get_bridge_errdisable_timeout_enable(msg->bridge_name,
                                                           &(replymsg.enabled));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;

        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;

      case SMI_MSG_MSTP_SETBRIDGE_TIMEOUTINT:
        status=mstp_api_set_bridge_errdisable_timeout_interval(msg->bridge_name,                                                               msg->timeout);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_MSTP_GETBRIDGE_TIMEOUTINT:
        /* Errdisable timeout interval TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_TIMEOUTINT);

        status = mstp_api_get_bridge_errdisable_timeout_interval(
                                                         msg->bridge_name,
                                                         &(replymsg.timeout));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;

        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;

      case SMI_MSG_MSTP_SETMSTI_PORTPRIORITY:
        status = mstp_api_set_msti_port_priority(msg->bridge_name,
                                                 msg->if_name, msg->instance, 
                                                 msg->portpriority_br);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_MSTP_GETMSTI_PORTPRIORITY:
        /* Bridge Priority TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_BRPRIORITY);
      
        status = mstp_api_get_msti_port_priority(msg->bridge_name,
                                                 msg->if_name, msg->instance,
                                                 &(replymsg.portpriority_br));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;
                                                                                
        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;

      case SMI_MSG_MSTP_SETREVISION_NUMBER:
        status = mstp_api_revision_number(msg->bridge_name,
                                          msg->revision_nu);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_MSTP_GETREVISION_NUMBER:
        /* Revision number TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_REVISION);

        status = mstp_api_get_revision_number(msg->bridge_name,
                                              &(replymsg.revision_nu));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;
                                                                                
        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;

      case SMI_MSG_MSTP_SETAUTOEDGE:
        status = mstp_api_set_auto_edge(msg->bridge_name, msg->if_name,
                                        msg->auto_edge);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_MSTP_GETAUTOEDGE:
         /* Autoedge edge TLV, Extended Ctype */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_EXTENDED);
        SMI_SET_CTYPE (replymsg.extended_cindex_1, SMI_MSTP_CTYPE_AUTOEDGE);

        status = mstp_api_get_auto_edge(msg->bridge_name, msg->if_name,
                                        &(replymsg.auto_edge));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;

        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;

      case SMI_MSG_MSTP_SETREGIONNAME:
        status = mstp_api_region_name(msg->bridge_name, msg->region_name);
        ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                 NULL);
      break;

      case SMI_MSG_MSTP_GETREGIONNAME:
        /* Region name TLV, Extended TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_EXTENDED);
        SMI_SET_CTYPE (replymsg.extended_cindex_1, SMI_MSTP_CTYPE_REGIONNAME);

        status = mstp_api_get_region_name(msg->bridge_name, 
                                          replymsg.region_name);

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;

        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;

      case SMI_MSG_MSTP_GET_SPANNING_DETAILS:
        /* Spanning Tree TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_EXTENDED);
        SMI_SET_CTYPE (replymsg.extended_cindex_1, SMI_MSTP_CTYPE_SPANNINGTREE);

        status = mstp_api_get_spanning_tree_details (msg->bridge_name, 
                                                     &(replymsg.tree_details));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;

        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;

      case SMI_MSG_MSTP_GET_SPANNING_INTERFACE:

        ifp = ifg_lookup_by_name (&mstpm->ifg, msg->if_name);
        if (ifp == NULL)
        {
          SMI_FN_EXIT (mstp_smi_server_send_sync_message (ase, 
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }

        /* Port Details TLV */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_EXTENDED);
        SMI_SET_CTYPE (replymsg.extended_cindex_1, SMI_MSTP_CTYPE_PORT_DETAILS);

        status = mstp_api_get_spanning_tree_details_interface(ifp, 
                                                     &(replymsg.port_details));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;

        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;

      case SMI_MSG_MSTP_GET_SPANNING_MST:
      {
        /*Instance Port Details TLV, Extended Ctype */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_EXTENDED);
        SMI_SET_CTYPE (replymsg.extended_cindex_1, 
                       SMI_MSTP_CTYPE_INSTANCE_DETAILS);
        status = mstp_api_get_spanning_tree_mst(msg->bridge_name, 
                                                msg->instance,
                                                &(replymsg.instance_details));
        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;

        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      }
      break;
     
      case SMI_MSG_MSTP_GET_SPANNING_MST_CONF:
        /* Config Details TLV,Extended Ctype */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_EXTENDED);
        SMI_SET_CTYPE (replymsg.extended_cindex_1, 
                       SMI_MSTP_CTYPE_CONFIG_DETAILS);
        status = mstp_api_get_spanning_tree_mst_config(msg->bridge_name,
                                                    &replymsg.config_details);
        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;

        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
        break;

      case SMI_MSG_MSTP_STP_MSTDETAIL:
        /* Instance details TLV,Extended Ctype */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_EXTENDED);
        SMI_SET_CTYPE (replymsg.extended_cindex_1, 
                       SMI_MSTP_CTYPE_INSTANCE_DETAILS);

        status = mstp_api_get_spanning_tree_mstdetail (msg->bridge_name, 
                                                       msg->instance,
                                                  &(replymsg.instance_details));

        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;

        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
      break;

      case SMI_MSG_MSTP_STP_MSTDETAIL_IF:
        ifp = ifg_lookup_by_name (&mstpm->ifg, msg->if_name);
        if (ifp == NULL)
        {
          SMI_FN_EXIT (mstp_smi_server_send_sync_message (ase, 
                                                         SMI_MSG_STATUS,
                                                         SMI_ERROR, NULL));
        }
        /* Instance port details TLV, Extended Ctype */
        SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_EXTENDED);
        SMI_SET_CTYPE (replymsg.extended_cindex_1, 
                       SMI_MSTP_CTYPE_INSTANCEPORT_DETAILS);
        status = mstp_api_get_spanning_tree_mstdetail_interface (ifp, 
                                                             msg->instance,
                                              &(replymsg.instanceport_details));
        if(status < 0) /* failure */
            msgtype = SMI_MSG_STATUS;

        ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
        break;

        case SMI_MSG_MSTP_SETBRIDGE_BPDUFILTER :
          status = mstp_api_set_bridge_portfast_bpdufilter(msg->bridge_name,
                                                         msg->bridge_bpdufilter
                                                         );
          ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                   NULL);
        break;

        case SMI_MSG_MSTP_GETBRIDGE_BPDUFILTER:
          /* Port Bpdu Filter TLV */
          SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_EXTENDED);
          SMI_SET_CTYPE (replymsg.extended_cindex_1, 
                         SMI_MSTP_CTYPE_BRIDGEBPDUFILTER);

          status = mstp_api_get_bridge_portfast_bpdufilter(msg->bridge_name,
                                                 &(replymsg.bridge_bpdufilter));

          if(status < 0)
            msgtype = SMI_MSG_STATUS;

          ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                 &replymsg);
        break;

        case SMI_MSG_MSTP_CHECK:
          status = mstp_api_mcheck(msg->bridge_name, msg->if_name);
          ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                   NULL);
        break;

       case SMI_MSG_GET_BRIDGE_STATUS:
          /* Bridge forward TLV */
          SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_BRIDGEFORWARD);

          /* Extended CTYPE TLV */
          SMI_SET_CTYPE (replymsg.cindex, SMI_MSTP_CTYPE_EXTENDED);

          /* Non switch TLV */
          SMI_SET_CTYPE (replymsg.extended_cindex_1, 
                         SMI_MSTP_CTYPE_STP_ENABLED);

          status = mstp_api_get_bridge_status (msg->bridge_name,
                                               &(replymsg.stp_enabled),
                                               &(replymsg.br_forward));
          if(status < 0)
            msgtype = SMI_MSG_STATUS;

          ret = mstp_smi_server_send_sync_message (ase, msgtype, status,
                                                   &replymsg);
        break;
        case SMI_MSG_MSTP_HA_SWITCH:
#ifdef HAVE_HA
          status = fm_redun_state_set(HA_FM_PTR_GET(mstpm),
                                      msg->switch_to_state);
          if (status != FM_RC_OK)
            status = SMI_MSTP_SWITCH_ERR;
#else
          status = SMI_MSTP_SWITCH_ERR;
#endif /*HAVE_HA*/
          ret = mstp_smi_server_send_sync_message (ase, SMI_MSG_STATUS, status,
                                                   NULL);
        break;

       default:
       break;
    }
  SMI_FN_EXIT(ret);
}

int 
mstp_check_func_type (int functype)

{
  int ret=SMI_GET_FUNC;
  
  SMI_FN_ENTER ();

  switch(functype)
  {
    case SMI_MSG_MSTP_ADDINSTANCE:                     
      ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_DELINSTANCE:                    
        ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETAGE:                          
        ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_CHECK:                           
       ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_ADDPORT:                         
       ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_DELPORT:                        
        ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETHELLOTIME:                    
        ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETMAXAGE:                       
        ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETPORTEDGE:                    
        ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETVERSION:                     
        ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETPR:                           
        ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETFWDD:                       
        ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETMPR:                          
        ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETCOST:                         
        ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETRSTROLE:                      
      ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETRSTTCN:                      
      ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETPORT_PATHCOST:                
      ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETP2P:                          
      ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETPORT_HELLOTIME:              
      ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETPPR:                          
      ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETMAXHOPS:                     
      ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETPORT_PRIORITY:               
      ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETPORT_RESTRICT:                
      ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETPORT_RESTRICTTCN:             
      ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETPORT_ROOTGUARD:              
      ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETPORT_BPDUFILTER:              
      ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_ENABLE_BRIDGE:                   
      ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_DISABLE_BRIDGE:                  
      ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETPORT_BPDUGUARD:               
      ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SET_TXHOLDCOUNT:                 
      ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETBRIDGE_BPDUGUARD:             
      ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETBRIDGE_TIMEOUTEN:             
      ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETBRIDGE_TIMEOUTINT:           
      ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETMSTI_PORTPRIORITY:            
      ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETREVISION_NUMBER:              
      ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETAUTOEDGE:                     
      ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETREGIONNAME:                  
      ret=SMI_SET_FUNC;
    break;   
    case SMI_MSG_MSTP_SETBRIDGE_BPDUFILTER:           
      ret=SMI_SET_FUNC;
    break;   
    default:
    break;    
  }  
  SMI_FN_EXIT(ret);
}
