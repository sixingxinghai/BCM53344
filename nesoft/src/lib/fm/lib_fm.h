/*--------------------------------------------------------------------------
 * Copyright (C) 2007 All rights reserved
 *
 * File name:   lib_fm.h
 * Purpose:     To provide local to the lib/fm subdirectory definitions.
 *              The application should not include this file.
 *              The application should include lib_fm_api.h file.
 * Author:      STC
 * Data:        04/10/2008
 *----------------------------------------------------------------------------
 */

#ifndef _LIB_FM_H_
#define _LIB_FM_H_

#include "pal.h"
/* #include "lib_fm_gid.h" */
#include "lib_fm_api.h"


#define FM_MAX_SUBSCRIPTIONS 20

/*----------------------------------------------------------------------------
 * Name:        fmFaultDef_t
 * Purpose:     To contain fault registration parameters.
 * Members:
 *  ftd_GroupId   - Fault group id.
 *  ftd_FaultId   - Fault id - local to its group
 *  ftd_EventType - SAF defined event type
 *  ftd_Severity  - SAF defined severity level
 *  ftd_Recovery  - SAF defined recovery type
 *  ftd_ProbCause - SAF defined probable cause
    ftd_Name      - Fault name used for presentation purpose
 *----------------------------------------------------------------------------
 */
typedef struct fmFaultDef
{
  fmGroupId_e         ftd_GroupId;
  u_int16_t           ftd_FaultId;
  fmEventType_e       ftd_EventType;
  fmSeverity_e        ftd_Severity;
  fmRecovery_e        ftd_Recovery;
  fmProbableCause_e   ftd_ProbCause;
  char                ftd_Name[FM_MAX_FAULT_NAME+1];
} fmFaultDef_t;

/*----------------------------------------------------------------------------
 * Name:        fmFault_t
 * Purpose:     To represent a single fault
 *              All active faults are located on a d-linked list.
 *              Most recent fault is always at the head of the list
 *              The oldest fault is always at the back of the list
 *              When new occurence of already present fault, it is
 *              moved to teh beginning of teh list.
 *              When fault is cleared from the code, it is set to State eq. FALSE.
 *              From CLI a user can clear already cleared faults or all active faults.
 * Members:
 *      flt_pReg       - Pointer to user provided fault registration data
 *                       Key value: Local to a group fault identification
 *      flt_GroupId    - Key value: Identifier of the group where the fault belongs
 *      flt_DataLen    - Key value: If 0 => pData is the key, otherwise content of memory
 *                       pointed by pData is the key.
 *      flt_pData      - Key value: Contains a fault key value or points to memory with key value
 *      flt_State      - Fault state: set when fault reported, unset when cleared.
 *      flt_Cnt        - Number of fault occurences
 *      flt_FirstTime  - Time of first occurence
 *      flt_LastTime   - Time of last occurence
 *      flt_FileName   - File name of last occurence
 *      flt_FileLine   - Line number of last occurence
 *      flt_SeqNum     - Sequence number
 *----------------------------------------------------------------------------
 */
typedef struct fmFault
{
  fmFaultDef_t      *flt_pFaultDef;
  fmGroupId_e        flt_GroupId;
  u_int32_t          flt_DataLen;
  u_int8_t          *flt_pData;

  bool_t             flt_State;
  u_int32_t          flt_Cnt;
  struct pal_timeval flt_FirstTime;
  struct pal_timeval flt_LastTime;
  struct pal_timeval flt_ClearTime;
  char               flt_FileName[32];
  u_int32_t          flt_FileLine;
  u_int32_t          flt_SeqNum;
} fmFault_t;

/*----------------------------------------------------------------------------
 * Name:        fmGroup_t
 * Purpose:     To represent a group of faults
 * Members:
 *      pfgr_FaultTab-
 *      fgr_FaultCnt -
 *----------------------------------------------------------------------------
 */
typedef struct fmGroup
{
  fmFaultDef_t  *fgr_pFaultsDef;
  u_int16_t      fgr_Cnt;
  char           fgr_Name[FM_MAX_GROUP_NAME+1];
} fmGroup_t;

/*----------------------------------------------------------------------------
 * Name:        fmSubscription_t
 * Purpose:     To represent a single subscription request
 * Members:
 *      fms_SevLevel  - Severity level threshold
 *      fms_GroupId   - Group Identifier - FM_GID_ALL notify about all
 *                        at or above severity threshold level
 *      fms_NtfCb     - Notification callback
 *      fms_UserRef   - User (subscriber) reference
 *----------------------------------------------------------------------------
 */
typedef struct fmSubscr
{
  fmEventType_e  fms_EventType;
  fmSeverity_e   fms_SevLevel;
  fmGroupId_e    fms_GroupId;
  fmFaultNtfCb_t fms_NtfCb;
  intptr_t       fms_SubCtx;
} fmSubscr_t;

/*----------------------------------------------------------------------------
 * Name:        fmInstance_t
 * Purpose:     To represent the Fault Manager instance in the lib_globals.
 * Members:
 *      fmi_GrpRefTab - Ptr to array of pointers to fault groups
 *      fmi_SeqNum    - Sequence number - incremented with each fault occurrence
 *                        For presentation purpose.
 *      fmi_SubTab    - Subscription table
 *      fmi_SubTabCnt - Subscription table counter
 *      fmi_ActiveCnt - Total number active faults
 *----------------------------------------------------------------------------
 */
typedef struct fmInstance
{
  fmGroup_t     *fmi_GrpRefTab[FM_GID_MAX];
  u_int32_t      fmi_SeqNum;
  fmSubscr_t     fmi_SubTab[FM_MAX_SUBSCRIPTIONS];
  u_int16_t      fmi_SubTabCnt;
  u_int16_t      fmi_ActiveCnt;
  struct list    fmi_FaultList;
} fmInstance_t;

typedef enum fmCmdScope_e
{
  FM_SCOPE_ALL,
  FM_SCOPE_ACTIVE,
  FM_SCOPE_CLEARED
} fmCmdScope_e;


ZRESULT fmCliInit(struct lib_globals *pLib);


#endif /* _LIB_FM_H_ */

