/*========================================================================
** Copyright (C) 2006-2007   All Rights Reserved.
**
** mstp_smi_fm.c -- Fault management for SMI - MSTP.
**
**
===========================================================================*/

#include "lib_fm_api.h"
#include "smi_mstp_fm.h"

#ifdef HAVE_SMI

/* MSTP Faults */
fmFaultReg_t mstp_smi_fault_reg_tab[] =
{
   FM_DEF_PRO_CRIT_COMPRST(FM_MEMORY_ALLOCATION_FAILURE,
                           "Out of memory",            FM_PC_OUT_OF_MEMORY),
   FM_DEF_PRO_CRIT_COMPRST(MSTP_SMI_ALARM_NSM_SERVER_SOCKET_DISCONNECT,
                           "Server socket disconnect", FM_PC_COMMUNICATIONS_SUBSYSTEM_FAILURE),
   FM_DEF_PRO_CRIT_COMPRST(MSTP_SMI_ALARM_STP,
                           "STP Alarm",                FM_PC_APPLICATION_SUBSYSTEM_FAILURE),
   FM_DEF_PRO_CRIT_COMPRST(MSTP_SMI_ALARM_TRANSPORT_FAILURE,
                           "Transport failure",        FM_PC_COMMUNICATIONS_SUBSYSTEM_FAILURE)
};

/* Callback function to notify MSTP module about the fault. This function
   will capture the fault and send it to MSTP SMI client if client is up.
*/
void
_mstp_smi_fault_ntf_cb (intptr_t      sub_ctx,
                        fmGroupId_e   group,
                        u_int32_t     fault,
                        fmEventType_e event,
                        fmSeverity_e  sev,
                        fmRecovery_e  recovery,
                        fmProbableCause_e cause,
                        struct pal_timeval time,
                        u_int32_t     dataLen,
                        u_int8_t     *data,
                        char         *fname,
                        u_int32_t     fline)

{
  struct smi_msg_alarm alarm_msg;

  /* Check for connectivity with SMI client */
  if (mstp_smiserver && mstp_smiserver->info)
  {
    syslog(LOG_ERR, "mstp: sending alarm message");
    pal_mem_set (&alarm_msg, 0, sizeof(struct smi_msg_alarm));

    alarm_msg.cindex = 0;

    SMI_SET_CTYPE (alarm_msg.cindex, SMI_ALARM_CTYPE_MODULE_NAME);
    alarm_msg.smi_module = SMI_AC_MSTP_MODULE;


    switch (fault)
    {
      case FM_MEMORY_ALLOCATION_FAILURE:

        SMI_SET_CTYPE (alarm_msg.cindex, SMI_ALARM_CTYPE_ALARM_TYPE);
        alarm_msg.alarm_type = SMI_ALARM_MEMORY_FAILURE;

      break;

      case MSTP_SMI_ALARM_NSM_SERVER_SOCKET_DISCONNECT:

        SMI_SET_CTYPE (alarm_msg.cindex, SMI_ALARM_CTYPE_ALARM_TYPE);
        alarm_msg.alarm_type = SMI_ALARM_NSM_SERVER_SOCKET_DISCONNECT;

        SMI_SET_CTYPE (alarm_msg.cindex, SMI_ALARM_CTYPE_DATA_NSM_CLIENT);
        alarm_msg.nsm_client = SMI_NSM_CLIENT_MSTP;

      break;

      case MSTP_SMI_ALARM_STP:
      {
        SMI_SET_CTYPE (alarm_msg.cindex, SMI_ALARM_CTYPE_ALARM_TYPE);
        alarm_msg.alarm_type = SMI_ALARM_STP;

        SMI_SET_CTYPE (alarm_msg.cindex, SMI_ALARM_CTYPE_DATA_STP_ALARM);
        pal_mem_cpy (&alarm_msg.stp_alarm_info,
                    (struct smi_stp_alarm_info *) data,
                    sizeof(struct smi_stp_alarm_info));
      }
      break;

      case MSTP_SMI_ALARM_TRANSPORT_FAILURE:

        syslog(LOG_ERR, "MSTP_SMI_ALARM_TRANSPORT_FAILURE");
        SMI_SET_CTYPE (alarm_msg.cindex, SMI_ALARM_CTYPE_ALARM_TYPE);
        alarm_msg.alarm_type = SMI_ALARM_TRANSPORT_FAILURE;

        SMI_SET_CTYPE (alarm_msg.cindex, SMI_ALARM_CTYPE_DATA_TRANSPORT_DESC);
        pal_snprintf (alarm_msg.description, SMI_TRANSPORT_DESC_MAX,
                     (u_char *) data);

      break;

      default:
      break;
    }
    smi_server_send_alarm_msg (mstp_smiserver->info,
                             (struct smi_msg_alarm *)&alarm_msg, SMI_MSG_ALARM);

  }
  return;
}

/* Initialize Faults for MSTP */
int
mstp_smi_faults_init (struct lib_globals *zg)
{
  return fmRegisterGroup(zg,
                         FM_GID_MSTP,
                         "MSTP",
                         mstp_smi_fault_reg_tab,
                         FM_NUM_REG_FAULTS(mstp_smi_fault_reg_tab));

}

/* Initialize Fault management for MSTP */
int
mstp_smi_fm_init (struct lib_globals *zg)
{
  MSTP_SMI_FM_GLOBALS *fmg;

  if( !zg)
    return RESULT_ERROR;

  if (mstp_smi_faults_init(zg) != 0) {
    zlog_info(zg, "mstp_fm_init: Cannot initialize MSTP faults");
    return RESULT_ERROR;
  }

  fmg = XCALLOC(MTYPE_FM, sizeof(MSTP_SMI_FM_GLOBALS));
  fmg->fmg_zg = zg;
  fmg->fmg_sev_threshold = FM_SEV_INFO;

  if (fmg->fmg_sev_threshold < FM_SEV_MAX) {
    fmSubscribe(zg, FM_EVT_ALL, fmg->fmg_sev_threshold, FM_GID_MSTP,
                _mstp_smi_fault_ntf_cb, (intptr_t)fmg);
  }

  return RESULT_OK;
}

void
smi_capture_mstp_info (int stp_violation, char * ifname,
                       int32_t lineNum, char * fileName)
{
  struct smi_stp_alarm_info stp_alarm_info;

  pal_mem_set(&stp_alarm_info, 0, sizeof(struct smi_stp_alarm_info));

  pal_snprintf(stp_alarm_info.ifname, SMI_INTERFACE_NAMSIZ, ifname);
  SET_FLAG (stp_alarm_info.flags, stp_violation);
  smi_record_fault (mstpm, FM_GID_MSTP, MSTP_SMI_ALARM_STP,
                   lineNum, fileName, &stp_alarm_info);
  return;
}

#endif /* HAVE_SMI */
