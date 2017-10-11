/*========================================================================
** Copyright (C) 2006-2007   All Rights Reserved.
**
** nsm_smi_fm.c -- Fault management for SMI - NSM.
**
**
===========================================================================*/

#include "lib_fm_api.h"
#include "smi_nsm_fm.h"

#ifdef HAVE_SMI

/* NSM Faults */
fmFaultReg_t nsm_smi_fault_reg_tab[] =
{
   FM_DEF_PRO_CRIT_COMPRST(FM_MEMORY_ALLOCATION_FAILURE,
                           "Out of memory",                     FM_PC_OUT_OF_MEMORY),
   FM_DEF_PRO_CRIT_COMPRST(NSM_SMI_ALARM_NSM_SERVER_SOCKET_DISCONNECT,
                           "Server socket disconnect",          FM_PC_COMMUNICATIONS_SUBSYSTEM_FAILURE),
   FM_DEF_PRO_CRIT_COMPRST( NSM_SMI_ALARM_NSM_CLIENT_SOCKET_DISCONNECT,
                            "Client socket disconnect",         FM_PC_COMMUNICATIONS_SUBSYSTEM_FAILURE),
   FM_DEF_PRO_CRIT_COMPRST(NSM_SMI_ALARM_TRANSPORT_FAILURE,
                           "Transport failure",                 FM_PC_COMMUNICATIONS_SUBSYSTEM_FAILURE),
   FM_DEF_PRO_CRIT_COMPRST(NSM_SMI_ALARM_LOC,
                           "Loss of carrier",                   FM_PC_COMMUNICATIONS_SUBSYSTEM_FAILURE),
   FM_DEF_COM_UNKN_RECNONE(NSM_SMI_ALARM_NSM_VLAN_ADD_TO_PORT,
                           "VLAN ADD TO PORT ALARM",            FM_PC_INFORMATION_MODIFICATION_DETECTED),
   FM_DEF_COM_UNKN_RECNONE( NSM_SMI_ALARM_NSM_VLAN_DEL_FROM_PORT,
                            "VLAN DELETE FROM PORT ALARM",      FM_PC_INFORMATION_MODIFICATION_DETECTED),
   FM_DEF_COM_UNKN_RECNONE( NSM_SMI_ALARM_NSM_VLAN_PORT_MODE,
                            "VLAN PORT MODE CHANGE ALARM",      FM_PC_INFORMATION_MODIFICATION_DETECTED),
   FM_DEF_COM_UNKN_RECNONE( NSM_SMI_ALARM_NSM_BRIDGE_PROTO_CHANGE,
                            "BRIDGE PROTOCOL CHANGE ALARM",     FM_PC_INFORMATION_MODIFICATION_DETECTED)
};

/* Callback function to notify NSM module about the fault. This function
   will capture the fault and send it to NSM SMI client if client is up.
*/
void
_nsm_smi_fault_ntf_cb (intptr_t     sub_ctx,
                       fmGroupId_e  group,
                       u_int32_t    fault,
                       fmEventType_e  event,
                       fmSeverity_e sev,
                       fmRecovery_e recovery,
                       fmProbableCause_e cause,
                       struct pal_timeval time,
                       u_int32_t    dataLen,
                       u_int8_t    *data,
                       char        *fname,
                       u_int32_t    fline)

{
  struct smi_msg_alarm alarm_msg;

  /* Check for connectivity with SMI client */
  if (ng->nsm_smiserver && ng->nsm_smiserver->info)
  {
    pal_mem_set (&alarm_msg, 0, sizeof(struct smi_msg_alarm));

    alarm_msg.cindex = 0;

    SMI_SET_CTYPE (alarm_msg.cindex, SMI_ALARM_CTYPE_MODULE_NAME);
    alarm_msg.smi_module = SMI_AC_NSM_MODULE;

    SMI_SET_CTYPE (alarm_msg.cindex, SMI_ALARM_CTYPE_ALARM_TYPE);

    switch (fault)
    {
      case FM_MEMORY_ALLOCATION_FAILURE:
        alarm_msg.alarm_type = SMI_ALARM_MEMORY_FAILURE;
      break;
      case NSM_SMI_ALARM_NSM_CLIENT_SOCKET_DISCONNECT:
      {
        int *nsm_client = (int *) data;

        alarm_msg.alarm_type = SMI_ALARM_NSM_CLIENT_SOCKET_DISCONNECT;

        SMI_SET_CTYPE (alarm_msg.cindex, SMI_ALARM_CTYPE_DATA_NSM_CLIENT);
        alarm_msg.nsm_client = *nsm_client;
      }
      break;
      case NSM_SMI_ALARM_LOC:
      {
        char *ifname = (char *) data;

        alarm_msg.alarm_type = SMI_ALARM_LOC;

        SMI_SET_CTYPE (alarm_msg.cindex, SMI_ALARM_CTYPE_LOC_ALARM);
        pal_snprintf (alarm_msg.loc_alarm_info.ifname, SMI_INTERFACE_NAMSIZ,
                      ifname);
      }
      break;
      case NSM_SMI_ALARM_NSM_VLAN_ADD_TO_PORT:
      {
        alarm_msg.alarm_type = SMI_ALARM_NSM_VLAN_ADD_TO_PORT;

        SMI_SET_CTYPE (alarm_msg.cindex, SMI_ALARM_CTYPE_VLAN_ALARM);
        pal_mem_cpy (&alarm_msg.vlan_alarm_info,
                     (struct smi_vlan_port_alarm *) data,
                     sizeof(struct smi_vlan_port_alarm));
      }
      break;
      case NSM_SMI_ALARM_NSM_VLAN_DEL_FROM_PORT:
      {
        alarm_msg.alarm_type = SMI_ALARM_NSM_VLAN_DEL_FROM_PORT;

        SMI_SET_CTYPE (alarm_msg.cindex, SMI_ALARM_CTYPE_VLAN_ALARM);
        pal_mem_cpy (&alarm_msg.vlan_alarm_info,
                     (struct smi_vlan_port_alarm *) data,
                     sizeof(struct smi_vlan_port_alarm));
      }
      break;
      case NSM_SMI_ALARM_NSM_VLAN_PORT_MODE:
      {
        alarm_msg.alarm_type = SMI_ALARM_NSM_VLAN_PORT_MODE;

        SMI_SET_CTYPE (alarm_msg.cindex, SMI_ALARM_CTYPE_VLAN_PORT_MODE_ALARM);
        pal_mem_cpy (&alarm_msg.vlan_port_mode_alarm_info,
                     (struct smi_vlan_port_mode_alarm *) data,
                     sizeof(struct smi_vlan_port_mode_alarm));
      }
      break;
      case NSM_SMI_ALARM_NSM_BRIDGE_PROTO_CHANGE:
      {
        alarm_msg.alarm_type = SMI_ALARM_NSM_BRIDGE_PROTO_CHANGE;

        SMI_SET_CTYPE (alarm_msg.cindex,
                       SMI_ALARM_CTYPE_BRIDGE_PROTOCOL_CHANGE_ALARM);
        pal_mem_cpy (&alarm_msg.bridge_proto_change_alarm_info,
                     (struct smi_bridge_protocol_change_alarm *) data,
                     sizeof(struct smi_bridge_protocol_change_alarm));
      }
      break;

      default:
      break;
    }
    smi_server_send_alarm_msg (ng->nsm_smiserver->info,
                             (struct smi_msg_alarm *)&alarm_msg, SMI_MSG_ALARM);

  }
  return;
}

/* Initialize Faults for NSM */
int
nsm_smi_faults_init (struct lib_globals *zg)
{
  return fmRegisterGroup(zg,
                         FM_GID_NSM,
                         "NSM",
                         nsm_smi_fault_reg_tab,
                         FM_NUM_REG_FAULTS(nsm_smi_fault_reg_tab));

}

/* Initialize Fault management for NSM */
int
nsm_smi_fm_init (struct lib_globals *zg)
{
  NSM_SMI_FM_GLOBALS *fmg;

  if( !zg)
    return RESULT_ERROR;

  if (nsm_smi_faults_init(zg) != 0) {
    zlog_info(zg, "nsm_fm_init: Cannot initialize NSM faults");
    return RESULT_ERROR;
  }

  fmg = XCALLOC(MTYPE_FM, sizeof(NSM_SMI_FM_GLOBALS));
  fmg->fmg_zg = zg;
  fmg->fmg_sev_threshold = FM_SEV_INFO;

  if (fmg->fmg_sev_threshold < FM_SEV_MAX) {
    fmSubscribe(zg, FM_EVT_ALL, fmg->fmg_sev_threshold, FM_GID_NSM,
                _nsm_smi_fault_ntf_cb, (intptr_t)fmg);
    fmSubscribe(zg, FM_EVT_ALL, fmg->fmg_sev_threshold, FM_GID_ONM,
                _nsm_smi_fault_ntf_cb, (intptr_t)fmg);
    fmSubscribe(zg, FM_EVT_ALL, fmg->fmg_sev_threshold, FM_GID_RMON,
                _nsm_smi_fault_ntf_cb, (intptr_t)fmg);
  }

  return RESULT_OK;
}

int
smi_record_vlan_port_mode_alarm (char *ifname, enum nsm_vlan_port_mode mode,
                                 enum nsm_vlan_port_mode submode)
{
  struct smi_vlan_port_mode_alarm data;

  pal_mem_set(&data, 0, sizeof (struct smi_vlan_port_mode_alarm));

  pal_snprintf(data.ifname, SMI_INTERFACE_NAMSIZ, ifname);
  data.mode = mode;
  data.sub_mode = submode;

  smi_record_fault (nzg, FM_GID_NSM, NSM_SMI_ALARM_NSM_VLAN_PORT_MODE,
                    __LINE__, __FILE__, &(data));

  return SMI_SUCEESS;
}

int
smi_record_bridge_proto_change_alarm (char *brname, enum smi_bridge_type type,
                                      enum smi_bridge_topo_type topo_type)
{
  struct smi_bridge_protocol_change_alarm data;

  pal_mem_set (&data, 0, sizeof (struct smi_bridge_protocol_change_alarm));

  pal_snprintf(data.brname, SMI_BRIDGE_NAMSIZ, brname);
  data.type = type;
  data.topo_type = topo_type;

  smi_record_fault (nzg, FM_GID_NSM, NSM_SMI_ALARM_NSM_BRIDGE_PROTO_CHANGE,
                    __LINE__, __FILE__, &(data));

  return SMI_SUCEESS;
}

#endif /* HAVE_SMI */
