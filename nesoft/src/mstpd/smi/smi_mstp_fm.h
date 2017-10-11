/*========================================================================
** Copyright (C) 2006-2007   All Rights Reserved.
**
** mstp_fm.h -- Fault management for MTSP.
**
**
===========================================================================*/

#ifndef _PACOS_MSTP_FM_H
#define _PACOS_MSTP_FM_H

#include "pal.h"
#include "lib.h"
#include "memory.h"

#ifdef HAVE_SMI
#include "smi_server.h"
#endif /* HAVE_SMI */

#define RESULT_OK     0
#define RESULT_ERROR  -1

/* MSTP faults */
enum mstp_smi_faults{
  MSTP_SMI_ALARM_MEMORY_FAILURE = FM_MEMORY_ALLOCATION_FAILURE,
  MSTP_SMI_ALARM_NSM_SERVER_SOCKET_DISCONNECT,
  MSTP_SMI_ALARM_TRANSPORT_FAILURE,
  MSTP_SMI_ALARM_STP,
};

/* MSTP FM global structure */
typedef struct mstp_smi_fm_globals {
  struct lib_globals *fmg_zg;
  void               *info;
  fmSeverity_e       fmg_sev_threshold;
} MSTP_SMI_FM_GLOBALS;

extern struct smi_server *mstp_smiserver;
extern struct lib_globals * mstpm;

/* Function prototypes */
void
_mstp_smi_fault_ntf_cb (intptr_t     sub_ctx,
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
                        u_int32_t     fline);

int
mstp_smi_faults_init(struct lib_globals *zg);

int
mstp_smi_fm_init(struct lib_globals *zg);

void
smi_capture_mstp_info (int stp_violation, char * ifname,
                       int32_t lineNum, char *fileName);

#endif /* _PACOS_MSTP_FM_H */
