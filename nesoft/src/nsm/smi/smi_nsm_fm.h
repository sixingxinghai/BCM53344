/*========================================================================
** Copyright (C) 2006-2007   All Rights Reserved.
**
** nsm_fm.h -- Fault management for NSM.
**
**
===========================================================================*/

#ifndef _PACOS_NSM_FM_H
#define _PACOS_NSM_FM_H

#include "pal.h"
#include "lib.h"
#include "memory.h"
#include "nsmd.h"

#ifdef HAVE_SMI
#include "smi_nsm_server.h"
/* Library includes */
#include "smi_server.h"
#endif

#define RESULT_OK     0
#define RESULT_ERROR  -1

/* NSM faults */
enum nsm_smi_faults{
  NSM_SMI_ALARM_MEMORY_FAILURE = FM_MEMORY_ALLOCATION_FAILURE,
  NSM_SMI_ALARM_NSM_SERVER_SOCKET_DISCONNECT,
  NSM_SMI_ALARM_NSM_CLIENT_SOCKET_DISCONNECT,
  NSM_SMI_ALARM_TRANSPORT_FAILURE,
  NSM_SMI_ALARM_LOC,
  NSM_SMI_ALARM_NSM_VLAN_ADD_TO_PORT,
  NSM_SMI_ALARM_NSM_VLAN_DEL_FROM_PORT,
  NSM_SMI_ALARM_NSM_VLAN_PORT_MODE,
  NSM_SMI_ALARM_NSM_BRIDGE_PROTO_CHANGE,
};

/* NSM FM global structure */
typedef struct nsm_smi_fm_globals {
  struct lib_globals *fmg_zg;
  void               *info;
  fmSeverity_e       fmg_sev_threshold;
} NSM_SMI_FM_GLOBALS;

extern struct nsm_globals *ng;

/* Function prototypes */
void
_nsm_smi_fault_ntf_cb (intptr_t     sub_ctx,
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
nsm_smi_faults_init(struct lib_globals *zg);

int
nsm_smi_fm_init(struct lib_globals *zg);

int
smi_record_bridge_proto_change_alarm (char *brname, enum smi_bridge_type type,
                                      enum smi_bridge_topo_type topo_type);

int
smi_record_vlan_port_mode_alarm (char *ifname, enum nsm_vlan_port_mode mode,
                                 enum nsm_vlan_port_mode submode);
#endif /* _PACOS_NSM_FM_H */
